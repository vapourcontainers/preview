<template>
  <a-config-provider :theme="{ algorithm: theme.darkAlgorithm, token: { borderRadius: 0 } }">
    <div :class="$style.preview">
      <img :src="preview" v-if="preview" :style="{ width: '100%' }" />
    </div>

    <a-modal v-model:open="scriptDialog" title="打开脚本" ok-text="确认" cancel-text="取消" @ok="loadScript">
      <a-input v-model:value="scriptPath" />
    </a-modal>

    <div :class="$style.toolbar">
      <a-slider v-model:value="frame" :max="maxFrame" :disabled="!videoInfo" @change="requestFrameThrottled" />

      <a-space :class="$style.control">
        <icon-button label="打开脚本" :icon="FolderOpenOutlined" primary :loading="scriptOpening" :compact="!!videoInfo"
          @click="openScript" />

        <icon-button label="重新加载脚本" :icon="ReloadOutlined" compact v-if="!!videoInfo" @click="loadScript" />

        <a-input-group compact>
          <icon-button :icon="CaretRightOutlined" compact :disabled="!videoInfo" @click="play" v-if="!playTimer" />
          <icon-button :icon="PauseOutlined" compact :disabled="!videoInfo" @click="pause" v-else />
          <icon-button :icon="FastBackwardOutlined" compact :disabled="!videoInfo || frame <= 0" @click="fastBackward" />
          <icon-button :icon="StepBackwardOutlined" compact :disabled="!videoInfo || frame <= 0" @click="stepBackward" />
          <icon-button :icon="StepForwardOutlined" compact :disabled="!videoInfo || frame >= maxFrame"
            @click="stepForward" />
          <icon-button :icon="FastForwardOutlined" compact :disabled="!videoInfo || frame >= maxFrame"
            @click="fastForward" />
        </a-input-group>

        <a-input-group compact>
          <a-input :class="$style.timecode" :style="{ width: '9em' }" :value="timestamp" :disabled="!videoInfo"
            readonly />
          <a-input :class="$style.timecode" :style="{ width: '7em' }" :value="fps" :disabled="!videoInfo" readonly />
        </a-input-group>

        <a-input-group compact>
          <a-input-number :class="$style.timecode" :style="{ width: '12em' }" v-model:value="frame"
            :addon-after="`/ ${maxFrame}`" :min="0" :max="maxFrame" :controls="false" :disabled="!videoInfo" />
          <icon-button label="转跳" :icon="ArrowRightOutlined" compact :disabled="!videoInfo" @click="requestFrame" />
        </a-input-group>

        <a-input-group compact>
          <a-input :class="$style.timecode" :style="{ width: '5em' }" :value="videoInfo?.width" :disabled="!videoInfo"
            readonly />
          <a-input :class="$style.timecode" :style="{ width: '5em' }" :value="videoInfo?.height" :disabled="!videoInfo"
            readonly />
        </a-input-group>
      </a-space>
    </div>
  </a-config-provider>
</template>

<script lang="ts" setup>
import { computed, ref } from 'vue';
import { theme } from 'ant-design-vue';
import {
  FolderOpenOutlined,
  ReloadOutlined,
  CaretRightOutlined,
  PauseOutlined,
  FastForwardOutlined,
  FastBackwardOutlined,
  StepForwardOutlined,
  StepBackwardOutlined,
  ArrowRightOutlined,
} from '@ant-design/icons-vue';
import io from 'socket.io-client';
import { throttle } from 'throttle-debounce';

import type { VSVideoInfo } from '@vscloud/native';

import IconButton from '@/components/IconButton.vue';

const socket = io('http://localhost:3000', {
  transports: ['websocket'],
  extraHeaders: {
    'Access-Control-Allow-Origin': '*',
  },
});

const frame = ref(0);
const videoInfo = ref<VSVideoInfo>();
const maxFrame = computed(() => videoInfo.value ? videoInfo.value.numFrames - 1 : 0);

const timestamp = computed(() => {
  if (!videoInfo.value) return '';

  const { fpsDen, fpsNum } = videoInfo.value;
  const fps = fpsNum / fpsDen;

  const s = frame.value / fps;
  const m = Math.floor(s / 60);
  const h = Math.floor(m / 60);

  const mm = (m % 60).toString().padStart(2, '0');
  const ss = (s % 60).toFixed(3).padStart(6, '0');

  return `${h}:${mm}:${ss}`;
});

const fps = computed(() => {
  if (!videoInfo.value) return '';

  const { fpsDen, fpsNum } = videoInfo.value;
  const fps = fpsNum / fpsDen;

  return fps.toFixed(3);
});

const scriptDialog = ref(false);
const scriptOpening = ref(false);
const scriptPath = ref('');

function openScript() {
  scriptDialog.value = true;
}

async function loadScript() {
  scriptOpening.value = true;
  scriptDialog.value = false;
  videoInfo.value = await socket.emitWithAck('open-file', scriptPath.value);
  scriptOpening.value = false;
  requestFrame();
}

async function fastBackward() {
  pause();
  frame.value = Math.max(frame.value - 10, 0);
  await requestFrame();
}

async function stepBackward() {
  pause();
  frame.value = Math.max(frame.value - 1, 0);
  await requestFrame();
}

async function stepForward() {
  pause();
  frame.value = Math.min(frame.value + 1, maxFrame.value);
  await requestFrame();
}

async function fastForward() {
  pause();
  frame.value = Math.min(frame.value + 10, maxFrame.value);
  await requestFrame();
}

const playTimer = ref<ReturnType<typeof setInterval>>();

async function play() {
  if (!videoInfo.value) return;
  const { fpsDen, fpsNum } = videoInfo.value;
  playTimer.value = setInterval(() => {
    if (frame.value < maxFrame.value) {
      frame.value++;
      requestFrame();
    } else {
      pause();
    }
  }, fpsDen / fpsNum * 1000);
}

async function pause() {
  if (playTimer.value) {
    clearInterval(playTimer.value);
    playTimer.value = undefined;
  }
}

const preview = ref('');

async function requestFrame() {
  const data = await socket.emitWithAck('request-frame', frame.value);
  if (data) {
    const blob = new Blob([data], { type: 'image/bmp' });
    preview.value = URL.createObjectURL(blob);
  } else {
    preview.value = '';
  }
}

const requestFrameThrottled = throttle(200, requestFrame);

document.addEventListener('keydown', (ev) => {
  if (ev.target instanceof HTMLButtonElement) {
    ev.preventDefault();
  }
  if (ev.target instanceof HTMLInputElement) {
    return;
  }
  switch (ev.key) {
    case 'ArrowLeft':
      if (ev.shiftKey) {
        fastBackward();
      } else {
        stepBackward();
      }
      break;
    case 'ArrowRight':
      if (ev.shiftKey) {
        fastForward();
      } else {
        stepForward();
      }
      break;
    case ' ':
      if (playTimer.value) {
        pause();
      } else {
        play();
      }
      break;
  }
});
</script>

<style lang="scss" module>
.preview {
  display: flex;
  padding-bottom: 48px;
  align-items: center;
  height: 100vh;
  width: 100vw;
  background: #303030;
  user-select: none;
}

.toolbar {
  position: absolute;
  left: 0;
  right: 0;
  bottom: 0;
  box-sizing: content-box;
  padding: 0 8px;
  background: #202021;

  .control {
    margin-bottom: 8px;
  }

  .timecode,
  .timecode :global(.ant-input-number-group-addon) {
    font-family: monospace;
  }
}
</style>
