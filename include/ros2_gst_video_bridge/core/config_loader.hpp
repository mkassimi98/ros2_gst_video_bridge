// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__CORE__CONFIG_LOADER_HPP_
#define ROS2_GST_VIDEO_BRIDGE__CORE__CONFIG_LOADER_HPP_

#include "ros2_gst_video_bridge/core/gst_bridge_config.hpp"

#include <rclcpp/rclcpp.hpp>

#include <string>
#include <vector>

namespace ros2_gst_video_bridge
{

class ConfigLoader
{
public:
  static GstBridgeConfig loadFromNode(rclcpp::Node & node);
  static std::vector<std::string> validate(const GstBridgeConfig & config);
  static std::string toDebugString(const GstBridgeConfig & config);

private:
  static void applyMachineProfileDefaults(GstBridgeConfig & config);
  static void applyStreamProfileDefaults(GstBridgeConfig & config);
};

}  // namespace ros2_gst_video_bridge

#endif  // ROS2_GST_VIDEO_BRIDGE__CORE__CONFIG_LOADER_HPP_
