# Versioning Policy

## Scope

This policy applies to:
- Runtime node behavior and parameters
- Pipeline profile semantics
- Interface contracts in ros2_gst_video_bridge_msgs

## SemVer Rules

- MAJOR: incompatible contract change (message field removals, parameter removals/renames without migration support).
- MINOR: backward-compatible additions (new parameters with defaults, new message fields preserving old behavior).
- PATCH: bug fixes and performance/stability improvements without contract break.

## Contract Stability

- `runtime_status` and `runtime_events` topics are versioned by package version.
- Config schema is tracked as `v1alpha`, then `v1` on first production release.
- Deprecated fields require at least one MINOR release overlap before removal.
