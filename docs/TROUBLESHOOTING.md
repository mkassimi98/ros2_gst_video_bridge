# Troubleshooting

## No Image Topic

Symptoms:
- `fps_in=0`
- no frames are sent to the receiver

Checks:

```bash
ros2 topic list
ros2 topic hz /camera/image_raw
ros2 topic echo /gst_video_bridge/runtime_status --once
```

Actions:
- Set `input_topic` to an existing `sensor_msgs/msg/Image` topic.
- Check camera driver namespace and remappings.
- Use `runtime.mode:=list_topics` to inspect visible image topics from the bridge host.

## Missing GStreamer Plugin

Symptoms:
- startup fails during pipeline creation
- `runtime_events` contains `PIPELINE_START_FAILED`
- logs mention an unknown element such as `srtsink`, `av1enc`, `x264enc`, or `nvv4l2av1enc`

Checks:

```bash
gst-inspect-1.0 appsrc
gst-inspect-1.0 srtsink
gst-inspect-1.0 av1enc
gst-inspect-1.0 x264enc
gst-inspect-1.0 nvv4l2av1enc
gst-inspect-1.0 nvv4l2h264enc
```

Actions:
- Install the required GStreamer plugin package for the selected transport/codec.
- Use `codec.name:=auto` to let the bridge choose from detected encoders.
- On Jetson, confirm NVIDIA multimedia packages are installed for `nvv4l2*` encoders.

## SRT Receiver Cannot Connect

Symptoms:
- `fps_in>0` but the receiver shows no video
- reconnect counters increase
- receiver times out

Checks:

```bash
ros2 topic echo /gst_video_bridge/runtime_status --once
ss -lntup | grep 9000
```

Actions:
- Use complementary SRT modes: bridge as `listener`, receiver as `caller`, or the reverse.
- Open firewall rules for the selected UDP port.
- Keep the same host/port in `transport.sink_uri` and the receiver URI.

## Hardware Encoder Fails

Symptoms:
- first-failure snapshot reports a hardware encoder
- bridge switches to software fallback
- startup or reconnect succeeds only after fallback

Checks:

```bash
ros2 run ros2_gst_video_bridge gst_video_bridge_node --ros-args -p runtime.mode:=list_capabilities
ros2 topic echo /gst_video_bridge/runtime_events
```

Actions:
- Keep `codec.name:=auto` unless a specific encoder is required.
- Use a software encoder explicitly for diagnosis, for example `codec.name:=av1 codec.encoder:=av1enc`.
- On Jetson, check camera format and whether the generated pipeline includes the required NVMM conversion.

## Bayer Image Looks Grayscale

Symptoms:
- stream is stable but color is missing
- logs mention Bayer grayscale fallback

Actions:
- Enable the launch debayer option:

```bash
ros2 launch ros2_gst_video_bridge gst_video_bridge_minimal.launch.py \
  enable_debayer:=true \
  input_topic:=/camera/image_raw \
  debayer_output_topic:=/camera/image_color
```

## High Latency or Drops

Symptoms:
- `dropped_backpressure_total` increases
- `push_latency_estimate_ms` or `push_latency_max_ms` is high
- receiver latency grows over time

Actions:
- Lower `codec.bitrate_kbps`.
- Lower `max_fps`.
- Use `runtime.adaptation.enabled:=true`.
- Prefer SRT for lossy links and UDP for simple low-latency local monitoring.
- Check CPU/GPU load and hardware encoder availability.
