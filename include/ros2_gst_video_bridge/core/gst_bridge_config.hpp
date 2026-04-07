// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__CORE__GST_BRIDGE_CONFIG_HPP_
#define ROS2_GST_VIDEO_BRIDGE__CORE__GST_BRIDGE_CONFIG_HPP_

#include <string>

namespace ros2_gst_video_bridge
{

struct GstBridgeConfig
{
  std::string input_topic{ "/camera/image_raw" };

  std::string gst_transport{ "srt" };
  std::string gst_codec{ "h264" };
  std::string gst_profile{ "low_latency" };
  std::string gst_sink_uri{ "srt://127.0.0.1:9000?mode=caller" };
  std::string gst_pipeline_override{};

  int gst_bitrate_kbps{ 2000 };
  int gst_latency_ms{ 60 };

  double max_fps{ 30.0 };
  bool use_wall_clock_timestamps{ false };
};

}  // namespace ros2_gst_video_bridge

#endif  // ROS2_GST_VIDEO_BRIDGE__CORE__GST_BRIDGE_CONFIG_HPP_
