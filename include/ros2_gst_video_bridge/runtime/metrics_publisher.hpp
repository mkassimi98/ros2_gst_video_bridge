// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__RUNTIME__METRICS_PUBLISHER_HPP_
#define ROS2_GST_VIDEO_BRIDGE__RUNTIME__METRICS_PUBLISHER_HPP_

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include <atomic>
#include <cstdint>
#include <string>

namespace ros2_gst_video_bridge
{

class MetricsPublisher
{
public:
  explicit MetricsPublisher(rclcpp::Node & node);

  void publishHeartbeat();
  void recordFrameIn(uint64_t now_ns, uint64_t source_timestamp_ns);
  void recordFrameOut();
  void recordFrameDropped();
  void recordReconnect();
  void updateRuntimeState(const std::string & runtime_state);

private:
  void publishSnapshot();

  rclcpp::Node & node_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr metrics_pub_;
  rclcpp::TimerBase::SharedPtr publish_timer_;

  std::atomic<uint64_t> frames_in_{0};
  std::atomic<uint64_t> frames_out_{0};
  std::atomic<uint64_t> frames_dropped_{0};
  std::atomic<uint64_t> reconnect_count_{0};

  uint64_t last_frames_in_{0};
  uint64_t last_frames_out_{0};
  uint64_t last_frames_dropped_{0};
  uint64_t last_publish_time_ns_{0};
  std::atomic<double> latency_estimate_ms_{0.0};
  std::string runtime_state_{"connecting"};
};

}  // namespace ros2_gst_video_bridge

#endif  // ROS2_GST_VIDEO_BRIDGE__RUNTIME__METRICS_PUBLISHER_HPP_
