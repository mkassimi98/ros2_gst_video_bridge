#!/usr/bin/env zsh
set -euo pipefail

source /opt/ros/humble/setup.zsh
source "${1:-/home/ccu-001/ws_dev}/install/setup.zsh"

declare -a codecs=(h264 h265 mjpeg)
declare -a transports=(srt udp rtsp file)

report="${2:-/tmp/ros2_gst_video_bridge_matrix.csv}"
echo "codec,transport,result,details" > "$report"

for codec in "${codecs[@]}"; do
  for transport in "${transports[@]}"; do
    sink_uri=""
    case "$transport" in
      srt) sink_uri="srt://127.0.0.1:9000?mode=listener" ;;
      udp) sink_uri="udp://127.0.0.1:5000" ;;
      rtsp) sink_uri="rtsp://127.0.0.1:8554/live" ;;
      file)
        ext="ts"
        [[ "$codec" == "mjpeg" ]] && ext="mkv"
        sink_uri="/tmp/bridge_${codec}.${ext}"
        ;;
    esac

    if timeout 10s ros2 run ros2_gst_video_bridge gst_video_bridge_node --ros-args \
      -p runtime.mode:=validate_config \
      -p codec.name:="$codec" \
      -p transport.kind:="$transport" \
      -p transport.sink_uri:="$sink_uri" >/tmp/bridge_matrix_case.log 2>&1; then
      echo "${codec},${transport},PASS,validate_config" >> "$report"
    else
      detail=$(tr '\n' ' ' </tmp/bridge_matrix_case.log | sed 's/,/;/g' | cut -c1-200)
      echo "${codec},${transport},FAIL,${detail}" >> "$report"
    fi
  done
done

echo "Matrix report written to: $report"
