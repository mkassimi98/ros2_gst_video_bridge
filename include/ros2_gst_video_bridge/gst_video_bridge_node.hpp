// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__GST_VIDEO_BRIDGE_NODE_HPP_
#define ROS2_GST_VIDEO_BRIDGE__GST_VIDEO_BRIDGE_NODE_HPP_

#include <rclcpp/rclcpp.hpp>

#include <string>

namespace ros2_gst_video_bridge
{

class GstVideoBridgeNode : public rclcpp::Node
{
public:
  explicit GstVideoBridgeNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  void logConfiguration() const;

  std::string input_topic_;
  std::string input_transport_;
  std::string pipeline_;
  double max_fps_;
  bool use_wall_clock_timestamps_;
};

}  // namespace ros2_gst_video_bridge

#endif  // ROS2_GST_VIDEO_BRIDGE__GST_VIDEO_BRIDGE_NODE_HPP_
