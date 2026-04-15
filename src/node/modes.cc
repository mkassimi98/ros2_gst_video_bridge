// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/gst_video_bridge_node.hpp"

#include "ros2_gst_video_bridge/core/config_loader.hpp"

#include <sstream>

namespace ros2_gst_video_bridge {

bool GstVideoBridgeNode::handleRuntimeMode() {
  if (config_.runtime.mode == "stream") {
    return false;
  }

  if (config_.runtime.mode == "list_topics") {
    printImageTopics();
    return true;
  }

  if (config_.runtime.mode == "list_capabilities") {
    printGstCapabilities();
    return true;
  }

  if (config_.runtime.mode == "validate_config") {
    RCLCPP_INFO(this->get_logger(), "Configuration is valid.");
    return true;
  }

  if (config_.runtime.mode == "discover") {
    printImageTopics();
    printGstCapabilities();
    RCLCPP_INFO(this->get_logger(), "Discovery mode completed successfully.");
    return true;
  }

  RCLCPP_ERROR(this->get_logger(), "Unsupported runtime.mode value: %s",
               config_.runtime.mode.c_str());
  return true;
}

void GstVideoBridgeNode::printImageTopics() const {
  const auto image_topics = topic_introspector_->listImageTopics();
  RCLCPP_INFO(this->get_logger(), "Detected image topics: %zu", image_topics.size());

  for (const auto& topic : image_topics) {
    std::ostringstream types;
    for (size_t i = 0; i < topic.second.size(); ++i) {
      types << topic.second[i];
      if (i + 1 < topic.second.size()) {
        types << ", ";
      }
    }
    RCLCPP_INFO(this->get_logger(), " - %s [%s]", topic.first.c_str(), types.str().c_str());
  }
}

void GstVideoBridgeNode::printGstCapabilities() const {
  if (!capability_probe_->hasGstInspect()) {
    RCLCPP_WARN(this->get_logger(),
                "gst-inspect-1.0 was not found on PATH. Reporting static fallback capabilities.");
  }

  const auto host_profile = capability_probe_->detectHostProfile();
  const auto plugins = capability_probe_->detectPlugins();
  const auto encoders = capability_probe_->detectEncoders();
  const auto encoder_impls = capability_probe_->detectEncoderImplementations();
  const auto sinks = capability_probe_->detectSinks();

  RCLCPP_INFO(this->get_logger(), "Detected host profile: %zu", host_profile.size());
  for (const auto& item : host_profile) {
    RCLCPP_INFO(this->get_logger(), " - %s", item.c_str());
  }

  RCLCPP_INFO(this->get_logger(), "Detected GStreamer plugins: %zu", plugins.size());
  for (const auto& plugin : plugins) {
    RCLCPP_INFO(this->get_logger(), " - %s", plugin.c_str());
  }

  RCLCPP_INFO(this->get_logger(), "Detected encoders: %zu", encoders.size());
  for (const auto& encoder : encoders) {
    RCLCPP_INFO(this->get_logger(), " - %s", encoder.c_str());
  }

  RCLCPP_INFO(this->get_logger(), "Detected encoder implementations (CPU/HW): %zu",
              encoder_impls.size());
  for (const auto& encoder_impl : encoder_impls) {
    RCLCPP_INFO(this->get_logger(), " - %s", encoder_impl.c_str());
  }

  RCLCPP_INFO(this->get_logger(), "Detected sinks/transports: %zu", sinks.size());
  for (const auto& sink : sinks) {
    RCLCPP_INFO(this->get_logger(), " - %s", sink.c_str());
  }
}

bool GstVideoBridgeNode::validateConfiguration() const {
  const auto errors = ConfigLoader::validate(config_);
  if (errors.empty()) {
    return true;
  }

  for (const auto& error : errors) {
    RCLCPP_ERROR(this->get_logger(), "Config error: %s", error.c_str());
  }

  return false;
}

} // namespace ros2_gst_video_bridge
