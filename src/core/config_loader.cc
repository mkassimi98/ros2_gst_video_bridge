// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/core/config_loader.hpp"

namespace ros2_gst_video_bridge
{

GstBridgeConfig ConfigLoader::loadFromNode(rclcpp::Node & node)
{
  GstBridgeConfig config;

  config.input_topic = node.declare_parameter<std::string>("input_topic", config.input_topic);
  config.gst_transport =
    node.declare_parameter<std::string>("gst.transport", config.gst_transport);
  config.gst_codec = node.declare_parameter<std::string>("gst.codec", config.gst_codec);
  config.gst_profile = node.declare_parameter<std::string>("gst.profile", config.gst_profile);
  config.gst_sink_uri =
    node.declare_parameter<std::string>("gst.sink_uri", config.gst_sink_uri);
  config.gst_pipeline_override =
    node.declare_parameter<std::string>("gst.pipeline_override", config.gst_pipeline_override);
  config.gst_bitrate_kbps =
    node.declare_parameter<int>("gst.bitrate_kbps", config.gst_bitrate_kbps);
  config.gst_latency_ms = node.declare_parameter<int>("gst.latency_ms", config.gst_latency_ms);
  config.max_fps = node.declare_parameter<double>("max_fps", config.max_fps);
  config.use_wall_clock_timestamps =
    node.declare_parameter<bool>("use_wall_clock_timestamps", config.use_wall_clock_timestamps);

  return config;
}

}  // namespace ros2_gst_video_bridge
