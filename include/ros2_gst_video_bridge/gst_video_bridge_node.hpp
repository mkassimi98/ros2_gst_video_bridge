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
#include <sensor_msgs/msg/image.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

namespace ros2_gst_video_bridge
{

class GstVideoBridgeNode : public rclcpp::Node
{
public:
  explicit GstVideoBridgeNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  ~GstVideoBridgeNode() override;

  bool hasImmediateExit() const;
  int immediateExitCode() const;

private:
  enum class RuntimeState : uint8_t
  {
    Connecting,
    Streaming,
    Degraded,
    Reconnecting,
    Failed
  };

  bool handleRuntimeMode();
  void printImageTopics() const;
  void printGstCapabilities() const;
  bool validateConfiguration() const;

  void initializeModules();
  void initializeSubscriptions();
  void initializeHealthMonitoring();
  void onImage(const sensor_msgs::msg::Image::SharedPtr msg);
  void runHealthCheck();
  void scheduleReconnect(const std::string & reason);
  bool tryReconnect();
  void setRuntimeState(RuntimeState new_state, const std::string & reason = "");
  static const char * runtimeStateToString(RuntimeState state);

  void logConfiguration() const;
  void logRuntimeCapabilities() const;

  GstBridgeConfig config_;
  std::string effective_pipeline_;
  int immediate_exit_code_{-1};

  std::unique_ptr<TopicIntrospector> topic_introspector_;
  std::unique_ptr<CapabilityProbe> capability_probe_;
  std::unique_ptr<StreamEngine> stream_engine_;
  std::unique_ptr<MetricsPublisher> metrics_publisher_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_subscription_;
  rclcpp::TimerBase::SharedPtr health_timer_;

  std::chrono::steady_clock::time_point last_frame_steady_time_{};
  std::chrono::steady_clock::time_point last_reconnect_attempt_{};
  RuntimeState runtime_state_{RuntimeState::Failed};
  bool reconnect_requested_{false};
  int reconnect_attempts_{0};
};

}  // namespace ros2_gst_video_bridge

#endif  // ROS2_GST_VIDEO_BRIDGE__GST_VIDEO_BRIDGE_NODE_HPP_
