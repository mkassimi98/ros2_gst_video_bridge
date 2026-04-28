# Launch Configuration

The package exposes three launch entry points:

- `gst_video_bridge_minimal.launch.py`: no YAML baseline; all modern node parameters are
  configurable as launch arguments.
- `gst_video_bridge_advanced.launch.py`: same arguments as minimal, plus `params_file`.
  Values passed on the command line override values loaded from the YAML file.
- `gst_video_bridge.launch.py`: compatibility wrapper around the advanced launch.

The launch files intentionally use command-line friendly names such as `codec_name`; the node still
receives the namespaced ROS parameter, such as `codec.name`.

## Common Examples

SRT listener with automatic codec selection:

```bash
ros2 launch ros2_gst_video_bridge gst_video_bridge_minimal.launch.py \
  input_topic:=/camera_driver_uv_example/vis/image_raw \
  "sink_uri:=srt://0.0.0.0:9000?mode=listener"
```

Jetson/NVIDIA H.265 hardware encoder:

```bash
ros2 launch ros2_gst_video_bridge gst_video_bridge_minimal.launch.py \
  profile_machine:=jetson \
  profile_stream:=low_latency \
  input_topic:=/camera_driver_uv_example/vis/image_raw \
  codec_name:=h265 \
  codec_encoder:=nvv4l2h265enc \
  codec_bitrate_kbps:=4000 \
  "sink_uri:=srt://0.0.0.0:9000?mode=listener"
```

Advanced launch with a YAML baseline and one CLI override:

```bash
ros2 launch ros2_gst_video_bridge gst_video_bridge_advanced.launch.py \
  params_file:=/path/to/profile.yaml \
  codec_bitrate_kbps:=2500
```

Configuration validation without streaming:

```bash
ros2 launch ros2_gst_video_bridge gst_video_bridge_minimal.launch.py \
  runtime_mode:=validate_config \
  print_effective_config:=true
```

## Argument Reference

| Launch argument | Node parameter | Type | Default |
|---|---|---|---|
| `profile_machine` | `profile.machine` | string | `generic` |
| `profile_stream` | `profile.stream` | string | `low_latency` |
| `input_topic` | `input_topic` | string | `/camera/image_raw` |
| `transport_kind` | `transport.kind` | string | `srt` |
| `sink_uri` | `transport.sink_uri` | string | `srt://127.0.0.1:9000?mode=listener` |
| `transport_latency_ms` | `transport.latency_ms` | int | `60` |
| `reconnect_enabled` | `transport.reconnect.enabled` | bool | `true` |
| `reconnect_interval_ms` | `transport.reconnect.interval_ms` | int | `1000` |
| `reconnect_max_attempts` | `transport.reconnect.max_attempts` | int | `0` |
| `codec_name` | `codec.name` | string | `auto` |
| `codec_encoder` | `codec.encoder` | string | empty |
| `codec_profile` | `codec.profile` | string | `baseline` |
| `codec_tune` | `codec.tune` | string | `zerolatency` |
| `codec_rate_control` | `codec.rate_control` | string | `cbr` |
| `codec_bitrate_kbps` | `codec.bitrate_kbps` | int | `2000` |
| `codec_gop` | `codec.gop` | int | `30` |
| `max_fps` | `max_fps` | double | `30.0` |
| `use_wall_clock_timestamps` | `use_wall_clock_timestamps` | bool | `false` |
| `runtime_mode` | `runtime.mode` | string | `stream` |
| `print_effective_config` | `runtime.print_effective_config` | bool | `true` |
| `backpressure_reconnect_after_ms` | `runtime.backpressure.reconnect_after_ms` | int | `2000` |
| `backpressure_max_consecutive_drops` | `runtime.backpressure.max_consecutive_drops` | int | `60` |
| `stream_id` | `runtime.stream_id` | string | `default` |
| `hw_fallback_failures` | `runtime.hw_fallback_failures` | int | `3` |
| `adaptation_enabled` | `runtime.adaptation.enabled` | bool | `true` |
| `adaptation_profile` | `runtime.adaptation.profile` | string | `balanced` |
| `adaptation_interval_ms` | `runtime.adaptation.interval_ms` | int | `2000` |
| `adaptation_cooldown_ms` | `runtime.adaptation.cooldown_ms` | int | `5000` |
| `pipeline_override` | `gst.pipeline_override` | string | empty |

## Launch-Only Arguments

| Launch argument | Type | Default | Purpose |
|---|---|---|---|
| `enable_debayer` | bool | `false` | Starts `image_proc/debayer_node` before the bridge |
| `debayer_output_topic` | string | `/camera/image_color` | Bridge input when `enable_debayer:=true` |
| `params_file` | path | installed `default_params.yaml` | Advanced/wrapper launch only |

## Notes

- Boolean arguments accept normal ROS launch values such as `true` and `false`.
- Integer and floating-point arguments are passed as typed ROS parameters, not loose strings.
- `codec_encoder:=` can be left empty to let the bridge auto-select an encoder for the platform.
- `pipeline_override` bypasses generated pipelines; use it only for diagnostics or experimental
  GStreamer chains.
