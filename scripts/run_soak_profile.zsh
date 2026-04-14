#!/usr/bin/env zsh
set -euo pipefail

source /opt/ros/humble/setup.zsh
source "${1:-/home/ccu-001/ws_dev}/install/setup.zsh"

duration_sec="${2:-1800}"
profile_machine="${3:-generic}"
profile_stream="${4:-low_latency}"
input_topic="${5:-/camera/image_raw}"
sink_uri="${6:-srt://127.0.0.1:9000?mode=listener}"

log_file="/tmp/bridge_soak_${profile_machine}_${profile_stream}_$(date +%Y%m%d_%H%M%S).log"

timeout "${duration_sec}s" ros2 launch ros2_gst_video_bridge gst_video_bridge_minimal.launch.py \
  profile_machine:="${profile_machine}" \
  profile_stream:="${profile_stream}" \
  input_topic:="${input_topic}" \
  sink_uri:="${sink_uri}" \
  codec_name:=auto \
  runtime_mode:=stream \
  >"${log_file}" 2>&1 || true

echo "Soak run finished. Log: ${log_file}"
