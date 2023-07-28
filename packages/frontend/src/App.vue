<template>
  <div :class="$style.preview">
    <img :src="preview" v-if="preview" :style="{ width: '100%' }" />
  </div>

  <a-space :class="$style.toolbar">
    <icon-button label="打开脚本" :icon="FolderOpenOutlined" primary :loading="scriptOpening" :compact="!!scriptFile"
      @click="openScript" />

    <icon-button label="重新加载脚本" :icon="ReloadOutlined" compact v-if="!!scriptFile" @click="loadScript" />

    <a-input-group compact>
      <icon-button :icon="CaretRightOutlined" compact :disabled="!scriptFile" @click="play" v-if="!playTimer" />
      <icon-button :icon="PauseOutlined" compact :disabled="!scriptFile" @click="pause" v-else />
      <icon-button :icon="FastBackwardOutlined" compact :disabled="!scriptFile || frame <= 0" @click="fastBackward" />
      <icon-button :icon="StepBackwardOutlined" compact :disabled="!scriptFile || frame <= 0" @click="stepBackward" />
      <icon-button :icon="StepForwardOutlined" compact :disabled="!scriptFile || frame >= maxFrame"
        @click="stepForward" />
      <icon-button :icon="FastForwardOutlined" compact :disabled="!scriptFile || frame >= maxFrame"
        @click="fastForward" />
    </a-input-group>

    <a-input-group compact>
      <a-input-number :style="{ width: '12em' }" v-model:value="frame" :addon-after="`/ ${maxFrame}`" :min="0"
        :max="maxFrame" :controls="false" :disabled="!scriptFile" />
      <icon-button label="转跳" :icon="ArrowRightOutlined" compact :disabled="!scriptFile" @click="requestFrame" />
    </a-input-group>

    <div :style="{ flex: 'auto' }">
      <a-slider v-model:value="frame" :max="maxFrame" :disabled="!scriptFile"
        @change="() => throttle(100, requestFrame)()" />
    </div>
  </a-space>
</template>

<script lang="ts" setup>
import { computed, ref } from 'vue';
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
import { throttle } from 'throttle-debounce';

import type { VSVideoInfo } from '@vscloud/native';

import IconButton from '@/components/IconButton.vue';

const scriptFile = ref('');
const scriptOpening = ref(false);
async function openScript() {
}

const frame = ref(0);
const videoInfo = ref<VSVideoInfo>();
const maxFrame = computed(() => videoInfo.value ? videoInfo.value.numFrames - 1 : 0);

async function loadScript() {
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

const playTimer = ref<NodeJS.Timer>();
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
}

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
  padding: 8px;
  background: #202021;

  &:global(.ant-space) {
    display: flex;
  }

  :global(.ant-space-item:last-child) {
    flex: auto;
  }
}
</style>
