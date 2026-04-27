# Changelog

All notable changes to this project are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versioning follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased] — pre-release, branch `ft/1stversion_w_srt`

### Added

**Node architecture**
- Modular node split: `src/node/` with one file per responsibility (`core`, `lifecycle`, `modes`, `streaming`, `recovery`, `fallback`, `observability`). Each file stays under 300 lines.
- Separate `src/core/` (config loading and pipeline building) and `src/runtime/` (capability probe, stream engine, metrics, image format, topic introspector) layers.

**Codec & encoder selection**
- `codec.name:=auto` resolves the best available codec and encoder using `gst-inspect-1.0` output, scored by machine profile (`jetson`, `x86`, `raspi`, `generic`).
- Machine profile auto-detection from host CPU architecture when `profile.machine` is left as `generic`.
- Hardware-to-software encoder fallback after configurable consecutive failures (`runtime.hw_fallback_failures`, default 3).

**Transport × codec pipeline matrix**
- Full support for `h264`, `h265`, `mjpeg` codecs across `srt`, `udp`, `rtsp`, `file` sinks.
- Jetson NVMM path for `nvv4l2h264enc` / `nvv4l2h265enc` (CPU → I420 → nvvidconv → NVMM → encoder).
- Software-only path for `x264enc`, `x265enc`, `jpegenc`, `openh264enc`.

**Image format support**
- BGR8, RGB8, BGRA8, RGBA8, MONO8, MONO16 (`GRAY16_LE`).
- Bayer 8-bit and 16-bit encodings streamed as grayscale fallback with a throttled warning to use `image_proc/debayer` for colour output.
- `validateImageShape` validates width × height × step × data-size consistency before pushing frames.

**Adaptive resilience**
- Three-level bitrate/fps/gop adaptation loop (levels 0–2) triggered by `Degraded` / `Reconnecting` state transitions and recovered during sustained `Streaming`.
- Three adaptation profiles: `conservative`, `balanced`, `aggressive`.
- Configurable interval and cooldown (`runtime.adaptation.interval_ms`, `runtime.adaptation.cooldown_ms`).
- Pipeline reconfigure on adaptation policy change (stops/rebuilds engine with updated parameters).

**Observability**
- `~/runtime_status` (`ros2_gst_video_bridge_msgs/msg/RuntimeStatus`): typed status at 1 Hz including fps_in/fps_out, drop counters split by throttle/malformed/backpressure, reconnect count, latency EWMA, push latency estimate + observed max, adaptation profile/level, and codec/encoder/fallback flags.
- `~/runtime_events` (`ros2_gst_video_bridge_msgs/msg/RuntimeEvent`): asynchronous events for `PIPELINE_STARTED`, `PIPELINE_DEGRADED`, `RECONNECT_FAILED`, `ADAPTATION_APPLIED`, `ENCODER_FALLBACK_SW`, `FIRST_FAILURE_SNAPSHOT`, `OPERATOR_PROFILE_UPDATE`, and others.
- `~/runtime_metrics` (`std_msgs/msg/String`): backward-compatible legacy key=value string.
- `~/set_streaming_profile` (`ros2_gst_video_bridge_msgs/srv/SetStreamingProfile`): operator service to switch adaptation profile and optionally reset all counters at runtime.
- `FIRST_FAILURE_SNAPSHOT` event captures session ID, stream ID, codec, encoder, transport, sink URI, SW-fallback flag, and full effective pipeline on the first streaming failure.
- Session ID (UUID-like) and stream ID are tracked across all events and status messages.

**Runtime modes**
- `stream` (default): run the bridge.
- `list_topics`: print visible `sensor_msgs/msg/Image` topics and exit.
- `list_capabilities`: run `gst-inspect-1.0` and report detected plugins, encoders, and sinks.
- `validate_config`: validate parameters and exit with code 0 (valid) or 1 (invalid).
- `discover`: combines `list_topics` + `list_capabilities`.

**Reconnect**
- Automatic reconnect with configurable interval and optional maximum attempt limit.
- SW-fallback is attempted on reconnect if HW encoder has failed repeatedly.
- `pipeline_reconfigure_requested_` flag ensures a new `StreamEngine` is created with the updated pipeline after adaptation or fallback.

**Launch files**
- `gst_video_bridge_minimal.launch.py`: essential arguments only.
- `gst_video_bridge_advanced.launch.py`: full parameter surface + `params_file` support.
- `gst_video_bridge.launch.py`: compatibility wrapper.
- All launches support optional `image_proc/debayer` node via `enable_debayer` argument.

**Profile presets**
- Machine profiles: `jetson`, `x86`, `raspi`, `generic`.
- Stream profiles: `default`, `low_latency`, `low_bandwidth`, `high_quality`, `monitoring_udp`.
- Seven curated YAML profile files under `config/profiles/`.

**Testing**
- `test_config_loader`: profile defaults, invalid mode rejection, alias precedence (4 tests).
- `test_pipeline_builder`: SRT/UDP h264, HW encoder (Jetson nvv4l2), h265 × 4 transports, mjpeg × 4 transports (5 tests).
- `test_image_format`: shape validation, unsupported encoding, invalid step, truncated data, Bayer grayscale fallback (5 tests).
- `test_stream_engine`: synthetic frame pushed through `appsrc → fakesink` pipeline (1 test).
- `test_detail_utils`: `startsWith`, `extractElementName`, `selectSoftwareEncoderForCodec`, `computeAdaptationScales` — 18 cases covering all profiles and edge conditions.
- `test_metrics_publisher`: counter accumulation, state reflection, adaptation/encoding metadata in published `RuntimeStatus` (9 tests).
- Smoke tests: `validate_config`, `list_capabilities`, and `--show-args` for all three launch files.
- Total: **11 CTest entries** (6 gtest targets + 5 smoke tests), **42 individual test cases**.

**Tooling**
- `clang-format-15` with `.clang-format` (LLVM style, 100-column limit, C++17).
- `clang-tidy-15` with `.clang-tidy` (bugprone, performance, readability, modernize, cppcoreguidelines checks).
- Both tools discovered by versioned binary names in `CMakeLists.txt`; build continues gracefully if not found.
- CI (`build-and-test` + `nightly-matrix`) on Ubuntu 22.04 / ROS 2 Humble.
- CI `clang-format-check` job runs `--dry-run --Werror` on all headers and sources.
- Codec/transport matrix script (`run_transport_codec_matrix.zsh`) and soak script (`run_soak_profile.zsh`).

**Documentation**
- `docs/CONTROL_PLANE.md`: topic/service contract reference.
- `docs/DEPENDENCIES.md`: required and optional apt packages, plugin checks.
- `docs/PLATFORM_MATRIX.md`: tested hardware/OS/ROS combinations.
- `docs/TROUBLESHOOTING.md`: common failure modes with diagnosis and recovery steps.
- `README.md`: full parameter reference tables with types, defaults, and descriptions.
- `CONTRIBUTING.md`: development setup, coding conventions, test requirements, PR checklist.

### Changed

- `appsrc` configured as non-blocking (`block=false`) with a bounded 2-buffer internal queue and upstream leaky policy; back-pressure drops are counted separately and do not cause a reconnect when the queue is the only error.
- `busLoop` switched from a 100 ms blocking `gst_bus_timed_pop_filtered` call (held mutex) to a non-blocking poll + 10 ms sleep, eliminating push stalls under high frame rates.
- Modern `transport.*`, `codec.*`, `runtime.*` parameter names always override legacy `gst.*` aliases when both are set.
- CI checkout uses `path: ws/src/ros2_gst_video_bridge` matching a real colcon workspace layout.
- Internal duplicate utility functions (`startsWith`, `extractElementName`) extracted to `detail/string_utils.hpp`; adaptation scaling extracted to `detail/adaptation.hpp`; SW encoder selection to `detail/encoder_selection.hpp`.

### Fixed

- `busLoop` mutex stall: the bus thread no longer holds `mutex_` during a potentially blocking GStreamer poll.
- `buildLegacyPayload` now correctly declared and defined as `static`.
- Redundant `encoder{""}` default initializer removed from `CodecConfig`.
- `.gitignore` extended to exclude `TODO*.md` patterns.

### Verified

- Jetson Orin / aarch64, Linux 5.15.148-tegra, ROS Humble, GStreamer 1.20.x.
- Basler acA2440-35uc USB3 camera at ~30 Hz BayerRG8.
- MPEG-TS/H.264 file sink validated end-to-end.
- All 11 CTest tests pass on the Jetson build.
- `clang-format` check passes on all headers and sources.

---

## [0.1.0] — First tagged release (pending)

_See Unreleased above. This section will be finalised when the first annotated tag is created._

