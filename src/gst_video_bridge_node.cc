// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/gst_video_bridge_node.hpp"

namespace ros2_gst_video_bridge
{

GstVideoBridgeNode::GstVideoBridgeNode(const rclcpp::NodeOptions & options)
: Node("gst_video_bridge", options)
{
  input_topic_ = this->declare_parameter<std::string>("input_topic", "/camera/image_raw");
  input_transport_ = this->declare_parameter<std::string>("input_transport", "raw");
  pipeline_ = this->declare_parameter<std::string>(
    "pipeline",
    "appsrc ! videoconvert ! x264enc tune=zerolatency bitrate=2000 speed-preset=ultrafast ! "
    "mpegtsmux ! srtsink uri=srt://127.0.0.1:9000?mode=caller");
  max_fps_ = this->declare_parameter<double>("max_fps", 30.0);
  use_wall_clock_timestamps_ =
    this->declare_parameter<bool>("use_wall_clock_timestamps", false);

  logConfiguration();

  RCLCPP_INFO(
    this->get_logger(),
    "Skeleton node ready. Next step: subscribe to %s and push frames into a GStreamer appsrc.",
    input_topic_.c_str());
}

void GstVideoBridgeNode::logConfiguration() const
{
  RCLCPP_INFO(this->get_logger(), "input_topic: %s", input_topic_.c_str());
  RCLCPP_INFO(this->get_logger(), "input_transport: %s", input_transport_.c_str());
  RCLCPP_INFO(this->get_logger(), "max_fps: %.2f", max_fps_);
  RCLCPP_INFO(this->get_logger(), "use_wall_clock_timestamps: %s", use_wall_clock_timestamps_ ? "true" : "false");
  RCLCPP_INFO(this->get_logger(), "pipeline: %s", pipeline_.c_str());
}

}  // namespace ros2_gst_video_bridge
