# Control Plane Contract

## Topics

| Topic | Type | QoS Intent |
|---|---|---|
| `~/runtime_metrics` | `std_msgs/msg/String` | Reliable, volatile, keep last 10 |
| `~/runtime_status` | `ros2_gst_video_bridge_msgs/msg/RuntimeStatus` | Reliable, volatile, keep last 10 |
| `~/runtime_events` | `ros2_gst_video_bridge_msgs/msg/RuntimeEvent` | Reliable, volatile, keep last 50 |

## Services

| Service | Type | Purpose |
|---|---|---|
| `~/set_streaming_profile` | `ros2_gst_video_bridge_msgs/srv/SetStreamingProfile` | Switch adaptation profile and optionally reset counters |

## Contract Version

The current contract is `v1alpha` and is versioned by the package version. Breaking changes before
the first production release are allowed, but must be captured in `CHANGELOG.md` and
`docs/VERSIONING.md`.

## Status Fields

`RuntimeStatus` includes:

- stream identity: `session_id`, `stream_id`
- selected source/codec/encoder/transport
- runtime state
- fps in/out
- drop counters split by throttle, malformed input, and backpressure
- reconnect count
- latency estimate
- appsrc push latency estimate and observed max
- effective bitrate, GOP, and FPS
- codec auto-selection and fallback flags
- adaptation profile and level
