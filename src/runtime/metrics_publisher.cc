// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/runtime/metrics_publisher.hpp"

namespace ros2_gst_video_bridge
{

MetricsPublisher::MetricsPublisher(rclcpp::Node & node)
: node_(node)
{
}

void MetricsPublisher::publishHeartbeat() const
{
  RCLCPP_DEBUG(node_.get_logger(), "metrics heartbeat");
}

}  // namespace ros2_gst_video_bridge
