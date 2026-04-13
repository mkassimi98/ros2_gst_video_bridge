// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__RUNTIME__TOPIC_INTROSPECTOR_HPP_
#define ROS2_GST_VIDEO_BRIDGE__RUNTIME__TOPIC_INTROSPECTOR_HPP_

#include <rclcpp/rclcpp.hpp>

#include <map>
#include <string>
#include <vector>

namespace ros2_gst_video_bridge
{

class TopicIntrospector
{
public:
  explicit TopicIntrospector(rclcpp::Node & node);

  std::map<std::string, std::vector<std::string>> listTopics() const;
  std::map<std::string, std::vector<std::string>> listImageTopics() const;

private:
  rclcpp::Node & node_;
};

}  // namespace ros2_gst_video_bridge

#endif  // ROS2_GST_VIDEO_BRIDGE__RUNTIME__TOPIC_INTROSPECTOR_HPP_
