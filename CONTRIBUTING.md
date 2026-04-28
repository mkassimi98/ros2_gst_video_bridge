# Contributing

## Requirements

- ROS 2 Humble (Ubuntu 22.04 or Jetson JetPack 5.x)
- GStreamer 1.20+ with `gstreamer1.0-plugins-{base,good,bad}` and `gstreamer1.0-libav`
- `libgstreamer1.0-dev` and `libgstreamer-plugins-base1.0-dev`
- `clang-format-15` (optional but required to pass CI)

## Development Setup

```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws/src
git clone git@github.com:mkassimi98/ros2_gst_video_bridge.git
cd ~/ros2_ws
rosdep install --from-paths src --ignore-src -r -y
source /opt/ros/humble/setup.bash
colcon build --packages-up-to ros2_gst_video_bridge ros2_gst_video_bridge_msgs
source install/setup.bash
```

## Running Tests

```bash
# Build with tests enabled (default)
colcon build --packages-up-to ros2_gst_video_bridge \
  --cmake-args \
    -DROS2_GST_VIDEO_BRIDGE_ENABLE_CLANG_FORMAT_CHECK=OFF

# Run all tests and show results
colcon test --packages-select ros2_gst_video_bridge
colcon test-result --all --verbose
```

All 11 CTest entries must pass before opening a PR. If you add functionality, add a test for it.

## Code Style

This repository uses `clang-format-15` (LLVM base, 100-column limit, 2-space indent).

**Format check before every commit:**

```bash
# Dry-run — prints diffs without modifying files
find ros2_gst_video_bridge/include ros2_gst_video_bridge/src \
  \( -name '*.hpp' -o -name '*.cc' \) | \
  xargs clang-format-15 --dry-run --Werror
```

**Auto-fix formatting:**

```bash
find ros2_gst_video_bridge/include ros2_gst_video_bridge/src \
  \( -name '*.hpp' -o -name '*.cc' \) | \
  xargs clang-format-15 -i
```

**Style rules (summary):**

- All source code and comments in English.
- Keep files under ~300 lines; split by responsibility if they grow.
- Prefer small, testable, pure functions in `include/ros2_gst_video_bridge/detail/` headers.
- Keep runtime behaviour deterministic under overload.
- Do not remove backward-compatible parameters without a documented migration path and a deprecation warning at runtime.
- Avoid `using namespace` at file scope.
- `noexcept` on leaf functions that cannot throw.

## Branch Naming

| Prefix | Purpose |
|---|---|
| `ft/<slug>` | New feature |
| `fix/<slug>` | Bug fix |
| `doc/<slug>` | Documentation only |
| `ci/<slug>` | CI / tooling changes |
| `refactor/<slug>` | Code structure without behaviour change |

Use lowercase, hyphens as word separators (e.g. `ft/adaptation-profiles`).

## Commit Messages

Follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <imperative summary, ≤72 chars>

[optional body — what and why, not how]

[optional footer: BREAKING CHANGE, Closes #N]
```

**Types:** `feat`, `fix`, `docs`, `test`, `ci`, `refactor`, `perf`, `chore`  
**Scopes:** `node`, `runtime`, `config`, `pipeline`, `metrics`, `ci`, `docs`

Examples:
```
feat(runtime): add three-level adaptive bitrate/fps loop
fix(runtime): avoid busLoop holding pipeline mutex during blocking poll
test(detail): add adaptation scaling and SW encoder selection coverage
```

## PR Checklist

Before requesting review, confirm:

- [ ] `colcon build` succeeds without warnings on a clean build.
- [ ] All 11 CTest entries pass (`colcon test-result --all`).
- [ ] `clang-format-15 --dry-run --Werror` exits 0 on all modified files.
- [ ] New functionality has at least one test.
- [ ] README parameter table updated if new parameters were added.
- [ ] CHANGELOG.md `[Unreleased]` section updated.
- [ ] No `TODO*.md` files are staged (`git status` is clean of them).

## Platform Validation Reports

When validating a new platform, open an issue titled  
`[Platform] <OS> / <ROS> / <hardware>` and include:

- OS image and kernel version.
- GStreamer version (`gst-launch-1.0 --version`).
- Camera source, image encoding, and resolution.
- Available encoders from `ros2 run ros2_gst_video_bridge gst_video_bridge_node --ros-args -p runtime.mode:=list_capabilities`.
- Transport × codec combination tested.
- Runtime metrics after at least 60 seconds of streaming.

