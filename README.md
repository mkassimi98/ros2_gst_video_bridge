# ros2_gst_video_bridge
Generic ROS 2 video bridge that subscribes to a raw `sensor_msgs/Image` topic and forwards frames through configurable GStreamer pipelines (SRT, RTSP, RTP/UDP, files, and other sinks).

## Goals

- Keep transport generic by delegating output to configurable GStreamer pipelines.
- Keep ROS-side ingestion simple: raw image input, topic name configurable.
- Be production-friendly for Jetson/edge deployment and GCS streaming.

## Configuration Contract

- ROS side (minimal and stable):
	- `input_topic`

- GStreamer side (fully parametrizable):
	- `gst.transport`
	- `gst.codec`
	- `gst.profile`
	- `gst.sink_uri`
	- `gst.bitrate_kbps`
	- `gst.latency_ms`
	- `gst.pipeline_override`

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

## Current Status

This is a professional starter scaffold with formatting/linting and a minimal ROS 2 node.
The next implementation milestone is wiring ROS image subscriptions into a real GStreamer `appsrc` pipeline.
