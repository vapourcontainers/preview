#include "utils.h"

VSNode *
convert_to_rgb24(const VSAPI *vsapi, VSCore *core, VSNode *node)
{
	VSMap *args = vsapi->createMap();
	vsapi->mapSetNode(args, "clip", node, maAppend);
	vsapi->mapSetInt(args, "format", pfRGB24, maAppend);

	const char matrix_in[] = "709";
	vsapi->mapSetData(args, "matrix_in_s", matrix_in, sizeof(matrix_in), dtUtf8, maAppend);

	VSMap *ret = vsapi->invoke(vsapi->getPluginByID(VSH_RESIZE_PLUGIN_ID, core), "Point", args);
	node = vsapi->mapGetNode(ret, "clip", 0, NULL);

	vsapi->freeMap(ret);
	vsapi->freeMap(args);

	return node;
}

ssize_t
bitmap_get_size(const VSAPI *vsapi, const VSFrame *frame)
{
	const VSVideoFormat *vf = vsapi->getVideoFrameFormat(frame);
	int width = vsapi->getFrameWidth(frame, 0);
	int height = vsapi->getFrameHeight(frame, 0);
	int numPlanes = vf->numPlanes;
	return sizeof(bitmap_t) + width * height * numPlanes;
}

void
bitmap_write(const VSAPI *vsapi, const VSFrame *frame, uint8_t *bitmap)
{
	const VSVideoFormat *vf = vsapi->getVideoFrameFormat(frame);
	int width = vsapi->getFrameWidth(frame, 0);
	int height = vsapi->getFrameHeight(frame, 0);
	int numPlanes = vf->numPlanes;

	bitmap_t *hdr = (bitmap_t *)bitmap;
	memset(hdr, 0, sizeof(bitmap_t));
	memcpy(hdr->file_header.magic, "BM", 2);
	hdr->file_header.file_size = sizeof(bitmap_t) + width * height * numPlanes;
	hdr->file_header.data_offset = sizeof(bitmap_t);
	hdr->dib_header.header_size = sizeof(hdr->dib_header);
	hdr->dib_header.width = width;
	hdr->dib_header.height = height;
	hdr->dib_header.planes = 1;
	hdr->dib_header.bits_per_pixel = 24;

	uint8_t *data = bitmap + sizeof(bitmap_t);

	const int rgbRemap[] = {2, 1, 0};
	for (int rp = 0; rp < numPlanes; rp++) {
		int p = rgbRemap[rp];
		ptrdiff_t stride = vsapi->getStride(frame, p);
		const uint8_t *read_ptr = vsapi->getReadPtr(frame, p);
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < stride; x++) {
				data[(height - y - 1) * width * numPlanes + x * numPlanes + rp] =
						read_ptr[y * stride + x];
			}
		}
	}
}
