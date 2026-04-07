// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__RUNTIME__METRICS_PUBLISHER_HPP_
#define ROS2_GST_VIDEO_BRIDGE__RUNTIME__METRICS_PUBLISHER_HPP_

#include <rclcpp/rclcpp.hpp>

namespace ros2_gst_video_bridge
{

class MetricsPublisher
{
public:
  explicit MetricsPublisher(rclcpp::Node & node);

  void publishHeartbeat() const;

private:
  rclcpp::Node & node_;
};

}  // namespace ros2_gst_video_bridge

#endif  // ROS2_GST_VIDEO_BRIDGE__RUNTIME__METRICS_PUBLISHER_HPP_
