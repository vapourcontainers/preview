from vapoursynth import core
import vapoursynth as vs

output = core.std.BlankClip(width=1920, height=1080, format=vs.YUV420P8)
output = core.text.FrameNum(output, scale=10)
output.set_output()
