import loadBinding from 'bindings';
import SegfaultHandler from 'segfault-handler';

export interface VSVideoInfo {
  fpsNum: number;
  fpsDen: number;
  width: number;
  height: number;
  numFrames: number;
}

export interface VSScript {
  videoInfo: VSVideoInfo;
  getFrameSync(n: number): Buffer;
  close(): void;
}

export interface VSRenderBinding {
  evaluateFile(file: string): VSScript;
}

SegfaultHandler.registerHandler();

const binding: VSRenderBinding = loadBinding('vscloud_native');
export const evaluateFile = binding.evaluateFile;
