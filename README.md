# ros2_gst_video_bridge
Generic ROS 2 video bridge that subscribes to Image or CompressedImage topics and forwards them through configurable GStreamer pipelines (SRT, RTSP, RTP/UDP, files, and other sinks).

## Goals

- Keep transport generic by delegating output to configurable GStreamer pipelines.
- Support both `sensor_msgs/Image` and `sensor_msgs/CompressedImage` inputs.
- Be production-friendly for Jetson/edge deployment and GCS streaming.

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
