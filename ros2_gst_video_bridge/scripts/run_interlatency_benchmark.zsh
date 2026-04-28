#!/usr/bin/env zsh
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <workspace> [duration_sec] [status_topic] [samples_csv] [summary_csv]" >&2
  exit 2
fi

workspace="$1"
duration_sec="${2:-60}"
status_topic="${3:-/gst_video_bridge/runtime_status}"
samples_csv="${4:-/tmp/bridge_interlatency_samples.csv}"
summary_csv="${5:-/tmp/bridge_interlatency_summary.csv}"

# Avoid setup script warnings when parent shell exports -u and these vars are undefined.
export AMENT_TRACE_SETUP_FILES="${AMENT_TRACE_SETUP_FILES:-}"
export COLCON_TRACE="${COLCON_TRACE:-}"
export AMENT_PYTHON_EXECUTABLE="${AMENT_PYTHON_EXECUTABLE:-$(command -v python3)}"
export COLCON_PYTHON_EXECUTABLE="${COLCON_PYTHON_EXECUTABLE:-$(command -v python3)}"

source /opt/ros/humble/setup.zsh
source "$workspace/install/setup.zsh"

python3 "$workspace/src/ros2_gst_video_bridge/ros2_gst_video_bridge/scripts/benchmark_interlatency.py" \
  --duration-sec "$duration_sec" \
  --status-topic "$status_topic" \
  --samples-csv "$samples_csv" \
  --summary-csv "$summary_csv"
