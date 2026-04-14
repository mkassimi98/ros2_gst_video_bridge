# ros2_gst_video_bridge
Generic ROS 2 video bridge that subscribes to a raw `sensor_msgs/Image` topic and forwards frames through configurable GStreamer pipelines (SRT, RTSP, RTP/UDP, files, and other sinks).

## Goals

- Keep transport generic by delegating output to configurable GStreamer pipelines.
- Keep ROS-side ingestion simple: raw image input, topic name configurable.
- Be production-friendly for Jetson/edge deployment and GCS streaming.

## Configuration Contract (Phase 2)

- Profiles (layered defaults):
	- `profile.machine` (`generic`, `jetson`, `x86`, `raspi`)
	- `profile.stream` (`default`, `low_latency`, `low_bandwidth`, `high_quality`, `monitoring_udp`)

- ROS side (minimal and stable):
	- `input_topic`

- Transport parameters:
	- `transport.kind` (`srt`, `rtsp`, `udp`, `file`)
	- `transport.sink_uri`
	- `transport.latency_ms`
	- `transport.reconnect.enabled`
	- `transport.reconnect.interval_ms`
	- `transport.reconnect.max_attempts`

- Codec parameters:
	- `codec.name` (`h264`, `h265`, `mjpeg`)
	- `codec.profile`
	- `codec.tune`
	- `codec.rate_control`
	- `codec.bitrate_kbps`
	- `codec.gop`

- Runtime parameters:
	- `max_fps`
	- `use_wall_clock_timestamps`
	- `runtime.mode` (`stream`, `list_topics`, `list_capabilities`, `validate_config`, `discover`)
	- `runtime.print_effective_config`

- Escape hatch:
	- `gst.pipeline_override` (when set, bypasses preset generation)

## Repository Structure

```
ros2_gst_video_bridge/
	config/                      # Runtime parameters (pipeline, topics, fps, etc.)
	include/ros2_gst_video_bridge/
	launch/
	src/
	.clang-format
	.clang-tidy
	.editorconfig
	CMakeLists.txt
	package.xml
```

## Build

```bash
cd <your_ws>
colcon build --packages-select ros2_gst_video_bridge
source install/setup.bash
```

## Run

```bash
ros2 launch ros2_gst_video_bridge gst_video_bridge.launch.py
```

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
cd /home/ccu-001/ws_dev
source /opt/ros/humble/setup.zsh
source install/setup.zsh

ros2 run camera_aravis2 camera_finder
```

### 2) Launch camera node (Terminal A)

```bash
source /opt/ros/humble/setup.zsh
source /home/ccu-001/ws_dev/install/setup.zsh
ros2 launch camera_aravis2 camera_driver_uv_example.launch.py guid:=Basler-2676016350B6-23285942
```

### 3) Launch bridge node in SRT listener mode (Terminal B)

```bash
source /opt/ros/humble/setup.zsh
source /home/ccu-001/ws_dev/install/setup.zsh
ros2 launch ros2_gst_video_bridge gst_video_bridge_minimal.launch.py \
	input_topic:=/camera_driver_uv_example/vis/image_raw \
	"sink_uri:=srt://0.0.0.0:9000?mode=listener"
```

### 4) Launch SRT receiver as caller (Terminal C)

```bash
gst-launch-1.0 -v srtsrc uri="srt://1.0.0.22:9000?mode=caller" latency=60 ! tsdemux ! h264parse ! avdec_h264 ! videoconvert ! autovideosink sync=false
```

### 5) Quick runtime checks

Check that the camera topic is publishing:

```bash
source /opt/ros/humble/setup.zsh
source /home/ccu-001/ws_dev/install/setup.zsh
ros2 topic hz /camera_driver_uv_example/vis/image_raw
```

Check bridge runtime metrics:

```bash
source /opt/ros/humble/setup.zsh
source /home/ccu-001/ws_dev/install/setup.zsh
ros2 topic echo /gst_video_bridge/runtime_metrics --once
```

If `fps_in` and `fps_out` are both greater than zero, the bridge is actively forwarding frames.

### Launch files

- `gst_video_bridge_minimal.launch.py`:
	- essential arguments only (`profile_machine`, `profile_stream`, `input_topic`, `sink_uri`)
- `gst_video_bridge_advanced.launch.py`:
	- full override surface for transport/codec/runtime plus `params_file`
- `gst_video_bridge.launch.py`:
	- compatibility wrapper to the advanced launch

Examples:

```bash
ros2 launch ros2_gst_video_bridge gst_video_bridge_minimal.launch.py \
	profile_machine:=jetson profile_stream:=low_latency \
	input_topic:=/camera_driver_uv/vis/image_raw \
	sink_uri:=srt://127.0.0.1:9000?mode=listener

ros2 launch ros2_gst_video_bridge gst_video_bridge_advanced.launch.py \
	params_file:=/home/ccu-001/ws_dev/src/ros2_gst_video_bridge/config/profiles/jetson_monitoring_udp.yaml
```

### Curated profile files

Under `config/profiles/`:

- `jetson_low_latency_srt.yaml`
- `x86_low_latency_srt.yaml`
- `raspi_low_latency_srt.yaml`
- `jetson_monitoring_udp.yaml`
- `x86_monitoring_udp.yaml`
- `raspi_monitoring_udp.yaml`
- `recording_file_sink.yaml`

### Runtime metrics

The node publishes runtime metrics on `~/runtime_metrics` (`std_msgs/msg/String`), including:

- state (`connecting|streaming|degraded|reconnecting|failed`)
- `fps_in`, `fps_out`
- dropped frame counters
- reconnect counter
- latency estimate in milliseconds

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
