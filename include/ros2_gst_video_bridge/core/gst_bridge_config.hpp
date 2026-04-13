// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__CORE__GST_BRIDGE_CONFIG_HPP_
#define ROS2_GST_VIDEO_BRIDGE__CORE__GST_BRIDGE_CONFIG_HPP_

#include <string>

namespace ros2_gst_video_bridge
{

struct ProfileConfig
{
  std::string machine{ "generic" };
  std::string stream{ "default" };
};

struct SourceConfig
{
  std::string input_topic{ "/camera/image_raw" };
};

struct TransportConfig
{
  std::string kind{ "srt" };
  std::string sink_uri{ "srt://127.0.0.1:9000?mode=caller" };
  int latency_ms{ 60 };

  bool reconnect_enabled{ true };
  int reconnect_interval_ms{ 1000 };
  int reconnect_max_attempts{ 0 };  // 0 means unlimited
};

struct CodecConfig
{
  std::string name{ "h264" };
  std::string profile{ "baseline" };
  std::string tune{ "zerolatency" };
  std::string rate_control{ "cbr" };

  int bitrate_kbps{ 2000 };
  int gop{ 30 };
};

struct RuntimeConfig
{
  double max_fps{ 30.0 };
  bool use_wall_clock_timestamps{ false };
  std::string mode{ "stream" };
  bool print_effective_config{ true };
};

struct GstConfig
{
  std::string pipeline_override{};
};

struct GstBridgeConfig
{
  ProfileConfig profile;
  SourceConfig source;
  TransportConfig transport;
  CodecConfig codec;
  RuntimeConfig runtime;
  GstConfig gst;
};

}  // namespace ros2_gst_video_bridge

#endif  // ROS2_GST_VIDEO_BRIDGE__CORE__GST_BRIDGE_CONFIG_HPP_
