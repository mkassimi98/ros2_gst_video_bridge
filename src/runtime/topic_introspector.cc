// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/runtime/topic_introspector.hpp"

namespace ros2_gst_video_bridge
{

TopicIntrospector::TopicIntrospector(rclcpp::Node & node)
: node_(node)
{
}

std::map<std::string, std::vector<std::string>> TopicIntrospector::listTopics() const
{
  return node_.get_topic_names_and_types();
}

}  // namespace ros2_gst_video_bridge
