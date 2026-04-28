# ros2_gst_video_bridge
Generic ROS 2 video bridge that subscribes to a raw `sensor_msgs/Image` topic and forwards frames through configurable GStreamer pipelines (SRT, RTSP, RTP/UDP, files, and other sinks).

## Goals

- Keep transport generic by delegating output to configurable GStreamer pipelines.
- Keep ROS-side ingestion simple: raw image input, topic name configurable.
- Be production-friendly for Jetson/edge deployment and GCS streaming.

## Parameter Reference

All parameters are declared at node startup. Modern namespaced names always take precedence over legacy `gst.*` aliases.

### Profile (layered defaults)

| Parameter | Type | Default | Description |
|---|---|---|---|
| `profile.machine` | `string` | `generic` | Machine class: `generic`, `jetson`, `x86`, `raspi` |
| `profile.stream` | `string` | `default` | Stream profile: `default`, `low_latency`, `low_bandwidth`, `high_quality`, `monitoring_udp` |

### Source

| Parameter | Type | Default | Description |
|---|---|---|---|
| `input_topic` | `string` | `/camera/image_raw` | ROS topic to subscribe to (`sensor_msgs/msg/Image`) |

### Transport

| Parameter | Type | Default | Description |
|---|---|---|---|
| `transport.kind` | `string` | `srt` | Sink type: `srt`, `rtsp`, `udp`, `file` |
| `transport.sink_uri` | `string` | `srt://127.0.0.1:9000?mode=listener` | Full destination URI for the selected sink |
| `transport.latency_ms` | `int` | `60` | SRT latency in milliseconds (ignored for non-SRT sinks) |
| `transport.reconnect.enabled` | `bool` | `true` | Enable automatic reconnect on stream failure |
| `transport.reconnect.interval_ms` | `int` | `1000` | Minimum time between reconnect attempts (ms) |
| `transport.reconnect.max_attempts` | `int` | `0` | Maximum reconnect attempts; `0` = unlimited |

Legacy aliases (still accepted, overridden by namespaced forms): `gst.transport`, `gst.sink_uri`, `gst.latency_ms`.

### Codec

| Parameter | Type | Default | Description |
|---|---|---|---|
| `codec.name` | `string` | `auto` | Codec: `auto`, `av1`, `h265`, `h264`, `mjpeg` (`auto` prefers codec order `av1` -> `h265` -> `h264`, then MJPEG fallback) |
| `codec.encoder` | `string` | `` | Override specific GStreamer encoder element (e.g. `nvv4l2av1enc`, `nvv4l2h265enc`, `x264enc`); empty = auto-select |
| `codec.profile` | `string` | `baseline` | H.264/H.265 profile where supported by the selected encoder |
| `codec.tune` | `string` | `zerolatency` | x264enc/x265enc tune string |
| `codec.rate_control` | `string` | `cbr` | Rate control mode: `cbr`, `vbr` |
| `codec.bitrate_kbps` | `int` | `2000` | Target bitrate in kbps |
| `codec.gop` | `int` | `30` | Keyframe interval (frames) |

Legacy aliases: `gst.codec`, `gst.profile`, `gst.bitrate_kbps`.

### Runtime

| Parameter | Type | Default | Description |
|---|---|---|---|
| `max_fps` | `double` | `30.0` | Maximum frames per second forwarded to GStreamer |
| `use_wall_clock_timestamps` | `bool` | `false` | Use node wall-clock instead of image header stamp for PTS |
| `runtime.mode` | `string` | `stream` | Operating mode: `stream`, `list_topics`, `list_capabilities`, `validate_config`, `discover` |
| `runtime.print_effective_config` | `bool` | `true` | Log the effective configuration at startup |
| `runtime.backpressure.reconnect_after_ms` | `int` | `2000` | Restart the pipeline when `appsrc` backpressure is sustained for this long |
| `runtime.backpressure.max_consecutive_drops` | `int` | `60` | Restart the pipeline after this many consecutive backpressure drops |
| `runtime.stream_id` | `string` | `default` | Stream identifier included in status/event messages |
| `runtime.hw_fallback_failures` | `int` | `3` | Consecutive failures before switching HW encoder to SW fallback |

### Adaptive Resilience

| Parameter | Type | Default | Description |
|---|---|---|---|
| `runtime.adaptation.enabled` | `bool` | `true` | Enable automatic bitrate/fps/gop adaptation on degraded state |
| `runtime.adaptation.profile` | `string` | `balanced` | Adaptation aggressiveness: `conservative`, `balanced`, `aggressive` |
| `runtime.adaptation.interval_ms` | `int` | `2000` | Minimum interval between adaptation steps (ms) |
| `runtime.adaptation.cooldown_ms` | `int` | `5000` | Cooldown before recovering quality back up (ms) |

### Escape Hatch

| Parameter | Type | Default | Description |
|---|---|---|---|
| `gst.pipeline_override` | `string` | `` | Full GStreamer pipeline string; bypasses all preset generation when non-empty |


## Repository Structure

```
ros2_gst_video_bridge/
	ros2_gst_video_bridge/       # Node package
		config/
		include/ros2_gst_video_bridge/
		launch/
		scripts/
		src/
		test/
		CMakeLists.txt
		package.xml
	ros2_gst_video_bridge_msgs/  # Runtime status/events/control interfaces
		msg/
		srv/
		CMakeLists.txt
		package.xml
	.github/
	docs/
	README.md
	LICENSE
```

## Build

Install common dependencies on Ubuntu 22.04 / ROS 2 Humble:

```bash
sudo apt-get update
sudo apt-get install -y \
  gstreamer1.0-tools \
  gstreamer1.0-plugins-base \
  gstreamer1.0-plugins-good \
  gstreamer1.0-plugins-bad \
  gstreamer1.0-libav \
  libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev
```

Clone into a ROS 2 workspace and build both packages:

```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws/src
git clone <repo-url> ros2_gst_video_bridge
cd ..
rosdep install --from-paths src --ignore-src -r -y
colcon build --packages-up-to ros2_gst_video_bridge
source install/setup.bash
```

```bash
cd <your_ws>
rosdep install --from-paths src --ignore-src -r -y
colcon build --packages-up-to ros2_gst_video_bridge
source install/setup.bash
```

## Run

```bash
ros2 launch ros2_gst_video_bridge gst_video_bridge.launch.py
```

## Supported ROS Distributions

| ROS distribution | Status |
|---|---|
| Humble | Tested on Ubuntu 22.04 / Jetson |
| Iron/Jazzy/Rolling | Not validated yet |

## Known-Good Platform Matrix

| Host | Camera | Input | Output | Status |
|---|---|---|---|---|
| Jetson / aarch64, Linux 5.15.148-tegra | Basler acA2440-35uc USB3 | 1920x1200 BayerRG8 at about 30 Hz | H.264 MPEG-TS file sink | Tested |

See `docs/PLATFORM_MATRIX.md` for details.

## End-to-End Basler USB Test (SRT Listener -> Caller)

This section documents the validated launch flow for a Basler USB camera with:

- camera node publishing ROS images,
- bridge node running SRT in listener mode,
- receiver running as SRT caller.

### 0) One-time USB udev rule for Basler

```bash
sudo tee /etc/udev/rules.d/99-basler-usb.rules >/dev/null <<'EOF'
SUBSYSTEM=="usb", ATTR{idVendor}=="2676", MODE="0666", GROUP="plugdev"
EOF

sudo udevadm control --reload-rules
sudo udevadm trigger
```

Reconnect the USB camera (or reboot) after applying the rule.

### 1) Verify camera detection

```bash
export WS=<your_ros2_workspace>
cd "$WS"
source /opt/ros/humble/setup.zsh
source install/setup.zsh

ros2 run camera_aravis2 camera_finder
```

### 2) Launch camera node (Terminal A)

```bash
export WS=<your_ros2_workspace>
source /opt/ros/humble/setup.zsh
source "$WS/install/setup.zsh"
ros2 launch camera_aravis2 camera_driver_uv_example.launch.py guid:=Basler-2676016350B6-23285942
```

### 3) Launch bridge node in SRT listener mode (Terminal B)

```bash
export WS=<your_ros2_workspace>
source /opt/ros/humble/setup.zsh
source "$WS/install/setup.zsh"
ros2 launch ros2_gst_video_bridge gst_video_bridge_minimal.launch.py \
	input_topic:=/camera_driver_uv_example/vis/image_raw \
	"sink_uri:=srt://0.0.0.0:9000?mode=listener"
```

To force H.265 with the Jetson/NVIDIA hardware encoder:

```bash
source /opt/ros/humble/setup.zsh
source /home/ccu-001/ws_dev/install/setup.zsh

ros2 launch ros2_gst_video_bridge gst_video_bridge_minimal.launch.py \
	input_topic:=/camera_driver_uv_example/vis/image_raw \
	codec_name:=h265 \
	codec_encoder:=nvv4l2h265enc \
	profile_machine:=jetson \
	profile_stream:=low_latency \
	"sink_uri:=srt://0.0.0.0:9000?mode=listener"
```

Receiver for H.265 over SRT/MPEG-TS:

```bash
gst-launch-1.0 -v srtsrc uri="srt://<bridge_ip>:9000?mode=caller" latency=60 ! \
	tsdemux ! h265parse ! avdec_h265 ! videoconvert ! autovideosink sync=false
```

For 8-bit Bayer cameras, the bridge now enables in-pipeline debayer automatically on first frame.
If you prefer an explicit ROS-side color topic, or if your camera publishes 16-bit Bayer, you can
still enable optional debayer before the bridge:

```bash
ros2 launch ros2_gst_video_bridge gst_video_bridge_minimal.launch.py \
	input_topic:=/camera_driver_uv_example/vis/image_raw \
	enable_debayer:=true \
	debayer_output_topic:=/camera_driver_uv_example/vis/image_color \
	"sink_uri:=srt://0.0.0.0:9000?mode=listener"
```

### 4) Launch SRT receiver as caller (Terminal C)

```bash
gst-launch-1.0 -v srtsrc uri="srt://1.0.0.22:9000?mode=caller" latency=60 ! tsdemux ! h264parse ! avdec_h264 ! videoconvert ! autovideosink sync=false
```

### 5) Quick runtime checks

Check that the camera topic is publishing:

```bash
export WS=<your_ros2_workspace>
source /opt/ros/humble/setup.zsh
source "$WS/install/setup.zsh"
ros2 topic hz /camera_driver_uv_example/vis/image_raw
```

Check bridge runtime metrics:

```bash
export WS=<your_ros2_workspace>
source /opt/ros/humble/setup.zsh
source "$WS/install/setup.zsh"
ros2 topic echo /gst_video_bridge/runtime_metrics --once
```

If `fps_in` and `fps_out` are both greater than zero, the bridge is actively forwarding frames.

### Launch files

- `gst_video_bridge_minimal.launch.py`:
	- direct launch without a YAML file; every modern transport, codec, runtime, adaptation,
	  debayer, and pipeline override can be changed from the command line
- `gst_video_bridge_advanced.launch.py`:
	- same command-line override surface plus `params_file` for a layered YAML baseline
- `gst_video_bridge.launch.py`:
	- compatibility wrapper to the advanced launch

See `docs/LAUNCH.md` for the complete argument list and parameter mapping.

Examples:

```bash
ros2 launch ros2_gst_video_bridge gst_video_bridge_minimal.launch.py \
	profile_machine:=jetson profile_stream:=low_latency \
	input_topic:=/camera_driver_uv/vis/image_raw \
	codec_name:=h265 codec_encoder:=nvv4l2h265enc \
	codec_bitrate_kbps:=4000 max_fps:=30.0 \
	sink_uri:=srt://127.0.0.1:9000?mode=listener

ros2 launch ros2_gst_video_bridge gst_video_bridge_advanced.launch.py \
	params_file:=<workspace>/src/ros2_gst_video_bridge/ros2_gst_video_bridge/config/profiles/jetson_monitoring_udp.yaml \
	runtime_mode:=validate_config print_effective_config:=true
```

### Curated profile files

Under `ros2_gst_video_bridge/config/profiles/`:

- `jetson_low_latency_srt.yaml`
- `x86_low_latency_srt.yaml`
- `raspi_low_latency_srt.yaml`
- `jetson_monitoring_udp.yaml`
- `x86_monitoring_udp.yaml`
- `raspi_monitoring_udp.yaml`
- `recording_file_sink.yaml`

### Runtime metrics

The node publishes runtime metrics on `~/runtime_metrics` (`std_msgs/msg/String`) for backward compatibility, and typed control-plane topics via `ros2_gst_video_bridge_msgs`:

- `~/runtime_status` (`ros2_gst_video_bridge_msgs/msg/RuntimeStatus`)
- `~/runtime_events` (`ros2_gst_video_bridge_msgs/msg/RuntimeEvent`)
- `~/set_streaming_profile` (`ros2_gst_video_bridge_msgs/srv/SetStreamingProfile`)

Fields include:

- state (`connecting|streaming|degraded|reconnecting|failed`)
- `fps_in`, `fps_out`
- dropped frame counters split by throttle, malformed input, and downstream backpressure
- reconnect counter
- latency estimate in milliseconds
- appsrc push latency estimate and observed max
- selected codec/encoder and fallback flags
- adaptation profile and current adaptation level

Operator runtime command example:

```bash
ros2 service call /gst_video_bridge/set_streaming_profile ros2_gst_video_bridge_msgs/srv/SetStreamingProfile "{adaptation_profile: balanced, reset_counters: false}"
```

First-failure snapshot:

- On the first streaming failure in a session, the bridge publishes `FIRST_FAILURE_SNAPSHOT` on `~/runtime_events`.
- Payload includes session/stream IDs, runtime state, selected codec/encoder, sink URI, fallback status, and effective pipeline string.

### Automatic codec selection (`codec.name:=auto`)

When `codec.name` is set to `auto`, the node inspects available encoder implementations via
`gst-inspect-1.0` and resolves to the best codec for the selected machine profile.

Selection order:

1. Codec priority: `av1` -> `h265` -> `h264` -> `mjpeg`.
2. Within the selected codec, machine-specific implementation preference:

| Machine profile | Preferred implementation classes |
|---|---|
| `jetson` | `hw:nvidia-v4l2` -> `hw:nvidia` -> other HW (`v4l2`/`omx`/`vaapi`) -> `sw` |
| `x86` | `hw:vaapi` -> `hw:qsv` -> `hw:v4l2` -> NVIDIA HW -> `sw` |
| `raspi` | `hw:v4l2` -> `hw:omx` -> `sw` |
| `generic` | `hw:vaapi` -> `hw:v4l2` -> NVIDIA HW -> `hw:omx` -> `sw` |

If `codec.name:=auto` picks a hardware encoder and runtime fails repeatedly while streaming,
the bridge now falls back automatically to a software encoder for the same codec.
This fallback is cross-platform (Jetson/x86/Raspberry/generic) and uses detected encoders from
`gst-inspect-1.0`, not Jetson-only logic.

Fallback sensitivity can be tuned with:

```bash
-p runtime.hw_fallback_failures:=3
```

Adaptive resilience controls:

```bash
-p runtime.adaptation.enabled:=true \
-p runtime.adaptation.profile:=balanced \
-p runtime.adaptation.interval_ms:=2000 \
-p runtime.adaptation.cooldown_ms:=5000
```

Supported adaptation profiles:

- `conservative`
- `balanced`
- `aggressive`

### Validation automation

Codec/transport matrix script:

```bash
<workspace>/src/ros2_gst_video_bridge/ros2_gst_video_bridge/scripts/run_transport_codec_matrix.zsh <workspace> /tmp/matrix.csv
```

Soak run script:

```bash
<workspace>/src/ros2_gst_video_bridge/ros2_gst_video_bridge/scripts/run_soak_profile.zsh <workspace> 1800 generic low_latency /camera/image_raw
```

Reference receiver pipelines:

Note: AV1 over MPEG-TS is specified by AOM, but GStreamer 1.20 `mpegtsmux`/`tsdemux` do not
advertise `video/x-av1` pads. For AV1 over SRT this bridge uses Matroska; H.264/H.265 keep
MPEG-TS.

```bash
# SRT AV1 Matroska receiver
gst-launch-1.0 -v srtsrc uri="srt://127.0.0.1:9000?mode=caller" latency=60 ! \
  matroskademux ! av1parse ! avdec_av1 ! videoconvert ! autovideosink sync=false

# SRT H.264 MPEG-TS receiver
gst-launch-1.0 -v srtsrc uri="srt://127.0.0.1:9000?mode=caller" latency=60 ! \
  tsdemux ! h264parse ! avdec_h264 ! videoconvert ! autovideosink sync=false

# SRT H.265 MPEG-TS receiver
gst-launch-1.0 -v srtsrc uri="srt://127.0.0.1:9000?mode=caller" latency=60 ! \
  tsdemux ! h265parse ! avdec_h265 ! videoconvert ! autovideosink sync=false

# UDP H.264 RTP receiver
gst-launch-1.0 -v udpsrc port=5000 caps="application/x-rtp,media=video,encoding-name=H264,payload=96" ! \
  rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! autovideosink sync=false

# UDP AV1 RTP receiver, requires an AV1 RTP depayloader in the local GStreamer install
gst-launch-1.0 -v udpsrc port=5000 caps="application/x-rtp,media=video,encoding-name=AV1,payload=96" ! \
  rtpav1depay ! av1parse ! avdec_av1 ! videoconvert ! autovideosink sync=false

# MJPEG over UDP RTP receiver
gst-launch-1.0 -v udpsrc port=5000 caps="application/x-rtp,media=video,encoding-name=JPEG,payload=26" ! \
  rtpjpegdepay ! jpegdec ! videoconvert ! autovideosink sync=false
```

Release/versioning policy documents:

- `docs/VERSIONING.md`
- `docs/RELEASE.md`
- `docs/DEPENDENCIES.md`
- `docs/TROUBLESHOOTING.md`
- `docs/PLATFORM_MATRIX.md`
- `docs/CONTROL_PLANE.md`

Example:

```bash
ros2 run ros2_gst_video_bridge gst_video_bridge_node --ros-args -p codec.name:=auto
```

### Discoverability modes (Phase 3)

- List ROS image topics visible on the host:

```bash
ros2 run ros2_gst_video_bridge gst_video_bridge_node --ros-args -p runtime.mode:=list_topics
```

- List detected GStreamer plugins, encoders, and sinks:

```bash
ros2 run ros2_gst_video_bridge gst_video_bridge_node --ros-args -p runtime.mode:=list_capabilities
```

- Validate effective configuration and exit:

```bash
ros2 run ros2_gst_video_bridge gst_video_bridge_node --ros-args -p runtime.mode:=validate_config
```

- Run full discoverability report (topics + capabilities) and exit:

```bash
ros2 run ros2_gst_video_bridge gst_video_bridge_node --ros-args -p runtime.mode:=discover
```

## Current Status

The node now supports ROS image ingestion into a real GStreamer `appsrc` pipeline with runtime modes,
reconnection policy, and profile-driven launch/configuration.
