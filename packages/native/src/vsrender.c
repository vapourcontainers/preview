#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>

#include "VSHelper4.h"
#include "VSScript4.h"
#include "node_api.h"
#include "utils.h"

const VSAPI *vsapi = NULL;
const VSSCRIPTAPI *vssapi = NULL;

typedef struct {
	VSScript *script;
	VSNode *node;
	const VSVideoInfo *videoInfo;
} script_context_t;

static napi_value
script_get_frame_sync(napi_env env, napi_callback_info info)
{
	size_t argc = 1;
	napi_value args[1];
	script_context_t *ctx;
	napi_get_cb_info(env, info, &argc, args, NULL, (void **)&ctx);
	if (argc != 1) {
		napi_throw_error(env, "Error", "Invalid number of arguments");
		return NULL;
	}

	const VSVideoInfo *vi = ctx->videoInfo;

	int n;
	napi_get_value_int32(env, args[0], &n);
	if (n < 0 && n >= vi->numFrames) {
		napi_throw_error(env, "Error", "Invalid frame number");
		return NULL;
	}

	char errorMsg[256];
	const VSFrame *frame = vsapi->getFrame(n, ctx->node, errorMsg, sizeof(errorMsg));
	if (frame == NULL) {
		napi_throw_error(env, "Error", errorMsg);
		return NULL;
	}

	// Only RGB24 is supported
	assert(vi->format.colorFamily == cfRGB);
	assert(vi->format.numPlanes == 3);

	uint8_t *frame_data;
	int frame_size = bitmap_get_size(vsapi, frame);

	napi_value buffer;
	napi_create_buffer(env, frame_size, (void **)&frame_data, &buffer);

	bitmap_write(vsapi, frame, frame_data);

	vsapi->freeFrame(frame);

	return buffer;
}

static napi_value
script_close(napi_env env, napi_callback_info info)
{
	script_context_t *ctx;
	napi_get_cb_info(env, info, NULL, NULL, NULL, (void **)&ctx);
	vsapi->freeNode(ctx->node);
	vssapi->freeScript(ctx->script);
	free(ctx);
	return NULL;
}

static void
populate_video_info(napi_env env, napi_value obj, const VSVideoInfo *vi)
{
	napi_value fpsNum;
	napi_create_int32(env, vi->fpsNum, &fpsNum);
	napi_set_named_property(env, obj, "fpsNum", fpsNum);

	napi_value fpsDen;
	napi_create_int32(env, vi->fpsDen, &fpsDen);
	napi_set_named_property(env, obj, "fpsDen", fpsDen);

	napi_value width;
	napi_create_int32(env, vi->width, &width);
	napi_set_named_property(env, obj, "width", width);

	napi_value height;
	napi_create_int32(env, vi->height, &height);
	napi_set_named_property(env, obj, "height", height);

	napi_value numFrames;
	napi_create_int32(env, vi->numFrames, &numFrames);
	napi_set_named_property(env, obj, "numFrames", numFrames);
}

static napi_value
evaluate_file(napi_env env, napi_callback_info info)
{
	size_t argc = 1;
	napi_value args[1];
	napi_get_cb_info(env, info, &argc, args, NULL, NULL);
	if (argc != 1) {
		napi_throw_error(env, "TypeError", "Invalid number of arguments");
		return NULL;
	}

	napi_valuetype valuetype;
	napi_typeof(env, args[0], &valuetype);
	if (valuetype != napi_string) {
		napi_throw_error(env, "TypeError", "Invalid argument type");
		return NULL;
	}

	char file[260];
	napi_get_value_string_utf8(env, args[0], file, sizeof(file), NULL);

	VSCore *core = vsapi->createCore(0);
	VSScript *script = vssapi->createScript(core);
	vssapi->evalSetWorkingDir(script, 1);

	if (vssapi->evaluateFile(script, file)) {
		const char *error = vssapi->getError(script);
		vssapi->freeScript(script);
		napi_throw_error(env, "ScriptError", error);
		return NULL;
	}

	VSNode *node = vssapi->getOutputNode(script, 0);
	if (!node) {
		vssapi->freeScript(script);
		return NULL;
	}

	node = convert_to_rgb24(vsapi, core, node);

	const VSVideoInfo *vi = vsapi->getVideoInfo(node);
	if (!vsh_isConstantVideoFormat(vi)) {
		vsapi->freeNode(node);
		vssapi->freeScript(script);
		return NULL;
	}

	napi_value result;
	napi_create_object(env, &result);

	napi_value videoInfo;
	napi_create_object(env, &videoInfo);
	populate_video_info(env, videoInfo, vi);
	napi_set_named_property(env, result, "videoInfo", videoInfo);

	script_context_t *ctx = malloc(sizeof(script_context_t));
	ctx->script = script;
	ctx->node = node;
	ctx->videoInfo = vi;

	napi_value func_get_frame_sync;
	napi_create_function(env, "getFrameSync", NAPI_AUTO_LENGTH, script_get_frame_sync, (void *)ctx,
			&func_get_frame_sync);
	napi_set_named_property(env, result, "getFrameSync", func_get_frame_sync);

	napi_value func_close;
	napi_create_function(env, "close", NAPI_AUTO_LENGTH, script_close, (void *)ctx, &func_close);
	napi_set_named_property(env, result, "close", func_close);

	return result;
}

NAPI_MODULE_INIT()
{
	char err_msg[256];

	dlopen("libvapoursynth-script" LIB_SUFFIX, RTLD_LAZY | RTLD_GLOBAL);

	vssapi = getVSScriptAPI(VSSCRIPT_API_VERSION);
	if (vssapi == NULL) {
		sprintf(err_msg, "VSScript API %08x is not supported", VSSCRIPT_API_VERSION);
		napi_throw_error(env, "Error", err_msg);
		return NULL;
	}

	vsapi = vssapi->getVSAPI(VAPOURSYNTH_API_VERSION);
	if (vsapi == NULL) {
		sprintf(err_msg, "VS API %08x is not supported", VAPOURSYNTH_API_VERSION);
		napi_throw_error(env, "Error", err_msg);
		return NULL;
	}

	napi_value exported_function;
	napi_create_function(
			env, "evaluateFile", NAPI_AUTO_LENGTH, evaluate_file, NULL, &exported_function);

	napi_set_named_property(env, exports, "evaluateFile", exported_function);

	return exports;
}
