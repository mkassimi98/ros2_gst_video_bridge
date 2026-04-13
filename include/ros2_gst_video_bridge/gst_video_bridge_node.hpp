// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__GST_VIDEO_BRIDGE_NODE_HPP_
#define ROS2_GST_VIDEO_BRIDGE__GST_VIDEO_BRIDGE_NODE_HPP_

#include "ros2_gst_video_bridge/core/gst_bridge_config.hpp"

#include "ros2_gst_video_bridge/runtime/capability_probe.hpp"
#include "ros2_gst_video_bridge/runtime/metrics_publisher.hpp"
#include "ros2_gst_video_bridge/runtime/stream_engine.hpp"
#include "ros2_gst_video_bridge/runtime/topic_introspector.hpp"

#include <rclcpp/rclcpp.hpp>

#include <memory>
#include <string>

namespace ros2_gst_video_bridge
{

class GstVideoBridgeNode : public rclcpp::Node
{
public:
  explicit GstVideoBridgeNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

  bool hasImmediateExit() const;
  int immediateExitCode() const;

private:
  bool handleRuntimeMode();
  void printImageTopics() const;
  void printGstCapabilities() const;
  bool validateConfiguration() const;

  void initializeModules();
  void logConfiguration() const;
  void logRuntimeCapabilities() const;

  GstBridgeConfig config_;
  std::string effective_pipeline_;
  int immediate_exit_code_{-1};

  std::unique_ptr<TopicIntrospector> topic_introspector_;
  std::unique_ptr<CapabilityProbe> capability_probe_;
  std::unique_ptr<StreamEngine> stream_engine_;
  std::unique_ptr<MetricsPublisher> metrics_publisher_;
};

}  // namespace ros2_gst_video_bridge

#endif  // ROS2_GST_VIDEO_BRIDGE__GST_VIDEO_BRIDGE_NODE_HPP_
