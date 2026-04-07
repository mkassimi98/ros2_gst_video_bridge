// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/runtime/capability_probe.hpp"

namespace ros2_gst_video_bridge
{

std::vector<std::string> CapabilityProbe::detectTransports() const
{
  // Phase 1: static baseline. Next phase: dynamic probe from gst-inspect output.
  return {"srt", "rtsp", "udp"};
}

std::vector<std::string> CapabilityProbe::detectCodecs() const
{
  // Phase 1: static baseline. Next phase: dynamic probe from gst-inspect output.
  return {"h264", "h265"};
}

}  // namespace ros2_gst_video_bridge
