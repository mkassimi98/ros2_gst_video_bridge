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

This is a professional starter scaffold with formatting/linting and a minimal ROS 2 node.
The next implementation milestone is wiring ROS image subscriptions into a real GStreamer `appsrc` pipeline.
