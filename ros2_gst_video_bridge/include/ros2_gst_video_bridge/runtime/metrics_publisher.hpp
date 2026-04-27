// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__RUNTIME__METRICS_PUBLISHER_HPP_
#define ROS2_GST_VIDEO_BRIDGE__RUNTIME__METRICS_PUBLISHER_HPP_

#include <rclcpp/rclcpp.hpp>
#include <ros2_gst_video_bridge_msgs/msg/runtime_event.hpp>
#include <ros2_gst_video_bridge_msgs/msg/runtime_status.hpp>
#include <std_msgs/msg/string.hpp>

#include <atomic>
#include <cstdint>
#include <string>

namespace ros2_gst_video_bridge {

class MetricsPublisher {
public:
  explicit MetricsPublisher(rclcpp::Node& node);

  void setSessionMetadata(const std::string& session_id, const std::string& stream_id,
                          const std::string& source_topic, const std::string& codec_name,
                          const std::string& encoder_name, const std::string& transport_kind,
                          const std::string& sink_uri);
  void setEffectiveEncoding(uint32_t bitrate_kbps, uint32_t gop, float max_fps);
  void setAdaptationState(const std::string& profile, uint8_t level);
  void setCodecSelectionFlags(bool auto_codec_requested, bool hw_selected,
                              bool sw_fallback_applied);
  void publishHeartbeat();
  void recordFrameIn(uint64_t now_ns, uint64_t source_timestamp_ns);
  void recordFrameOut();
  void recordFrameDroppedBackpressure();
  void recordFrameDroppedMalformed();
  void recordFrameDroppedThrottle();
  void recordPushLatency(double latency_ms);
  void recordReconnect();
  void updateRuntimeState(const std::string& runtime_state);
  void publishEvent(const std::string& severity, const std::string& code,
                    const std::string& message);

private:
  ros2_gst_video_bridge_msgs::msg::RuntimeStatus buildStatusSnapshot(uint64_t now_ns,
                                                                     double elapsed_s);
  static std::string buildLegacyPayload(
      const ros2_gst_video_bridge_msgs::msg::RuntimeStatus& status, uint64_t drops_since_last);
  void publishSnapshot();

  rclcpp::Node& node_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr metrics_pub_;
  rclcpp::Publisher<ros2_gst_video_bridge_msgs::msg::RuntimeStatus>::SharedPtr status_pub_;
  rclcpp::Publisher<ros2_gst_video_bridge_msgs::msg::RuntimeEvent>::SharedPtr event_pub_;
  rclcpp::TimerBase::SharedPtr publish_timer_;

  std::atomic<uint64_t> frames_in_{0};
  std::atomic<uint64_t> frames_out_{0};
  std::atomic<uint64_t> frames_dropped_backpressure_{0};
  std::atomic<uint64_t> frames_dropped_malformed_{0};
  std::atomic<uint64_t> frames_dropped_throttle_{0};
  std::atomic<uint64_t> reconnect_count_{0};

  uint64_t last_frames_in_{0};
  uint64_t last_frames_out_{0};
  uint64_t last_frames_dropped_{0};
  uint64_t last_publish_time_ns_{0};
  std::atomic<double> latency_estimate_ms_{0.0};
  std::atomic<double> push_latency_estimate_ms_{0.0};
  std::atomic<double> push_latency_max_ms_{0.0};
  std::string runtime_state_{"connecting"};

  std::string session_id_;
  std::string stream_id_{"default"};
  std::string source_topic_;
  std::string codec_name_;
  std::string encoder_name_;
  std::string transport_kind_;
  std::string sink_uri_;
  std::string adaptation_profile_{"balanced"};
  uint8_t adaptation_level_{0};
  bool auto_codec_requested_{false};
  bool hw_encoder_selected_{false};
  bool sw_fallback_applied_{false};

  uint32_t bitrate_kbps_effective_{0};
  uint32_t gop_effective_{0};
  float max_fps_effective_{0.0F};
};

} // namespace ros2_gst_video_bridge

#endif // ROS2_GST_VIDEO_BRIDGE__RUNTIME__METRICS_PUBLISHER_HPP_
