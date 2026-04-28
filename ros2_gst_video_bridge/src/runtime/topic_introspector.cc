// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/runtime/topic_introspector.hpp"

namespace ros2_gst_video_bridge {

TopicIntrospector::TopicIntrospector(rclcpp::Node& node) : node_(node) {}

std::map<std::string, std::vector<std::string>> TopicIntrospector::listTopics() const
{
    return node_.get_topic_names_and_types();
}

std::map<std::string, std::vector<std::string>> TopicIntrospector::listImageTopics() const
{
    const auto                                      all_topics = node_.get_topic_names_and_types();
    std::map<std::string, std::vector<std::string>> image_topics;

    for (const auto& topic : all_topics) {
        for (const auto& type : topic.second) {
            if (type == "sensor_msgs/msg/Image" || type == "sensor_msgs/msg/CompressedImage") {
                image_topics[topic.first] = topic.second;
                break;
            }
        }
    }

    return image_topics;
}

} // namespace ros2_gst_video_bridge
