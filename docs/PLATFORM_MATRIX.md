# Platform Matrix

## Tested

| Date | Host | OS / Kernel | ROS | GStreamer | Camera | Stream Path | Result |
|---|---|---|---|---|---|---|---|
| 2026-04-25 | Jetson / aarch64 | Linux 5.15.148-tegra | Humble | 1.20.x | Basler acA2440-35uc USB3 | ROS Image -> appsrc -> x264enc -> MPEG-TS file | Pass |

## Detected Encoders on Tested Jetson

| Codec | Software | Hardware |
|---|---|---|
| AV1 | `av1enc` | `nvv4l2av1enc` |
| H.264 | `x264enc`, `openh264enc` | `nvv4l2h264enc` |
| H.265 | `x265enc` | `nvv4l2h265enc` |
| MJPEG | `jpegenc` | `nvjpegenc` |

## Untested / Pending

| Host Class | Status |
|---|---|
| x86 desktop/server | Pending real hardware validation |
| Raspberry Pi | Pending real hardware validation |
| Generic Ubuntu VM/container | Pending validation without camera hardware |
