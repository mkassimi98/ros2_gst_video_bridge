// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/runtime/metrics_publisher.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace ros2_gst_video_bridge {

MetricsPublisher::MetricsPublisher(rclcpp::Node& node) : node_(node) {
  const auto status_qos = rclcpp::QoS(rclcpp::KeepLast(10)).reliable().durability_volatile();
  const auto event_qos = rclcpp::QoS(rclcpp::KeepLast(50)).reliable().durability_volatile();

  metrics_pub_ = node_.create_publisher<std_msgs::msg::String>("~/runtime_metrics", status_qos);
  status_pub_ = node_.create_publisher<ros2_gst_video_bridge_msgs::msg::RuntimeStatus>(
      "~/runtime_status", status_qos);
  event_pub_ = node_.create_publisher<ros2_gst_video_bridge_msgs::msg::RuntimeEvent>(
      "~/runtime_events", event_qos);

  last_publish_time_ns_ = static_cast<uint64_t>(node_.now().nanoseconds());
  publish_timer_ =
      node_.create_wall_timer(std::chrono::seconds(1), [this]() { publishSnapshot(); });
}

void MetricsPublisher::setSessionMetadata(
    const std::string& session_id, const std::string& stream_id, const std::string& source_topic,
    const std::string& codec_name, const std::string& encoder_name,
    const std::string& transport_kind, const std::string& sink_uri) {
  session_id_ = session_id;
  stream_id_ = stream_id;
  source_topic_ = source_topic;
  codec_name_ = codec_name;
  encoder_name_ = encoder_name;
  transport_kind_ = transport_kind;
  sink_uri_ = sink_uri;
}

void MetricsPublisher::setEffectiveEncoding(uint32_t bitrate_kbps, uint32_t gop, float max_fps) {
  bitrate_kbps_effective_ = bitrate_kbps;
  gop_effective_ = gop;
  max_fps_effective_ = max_fps;
}

void MetricsPublisher::setAdaptationState(const std::string& profile, uint8_t level) {
  adaptation_profile_ = profile;
  adaptation_level_ = level;
}

void MetricsPublisher::setCodecSelectionFlags(bool auto_codec_requested, bool hw_selected,
                                              bool sw_fallback_applied) {
  auto_codec_requested_ = auto_codec_requested;
  hw_encoder_selected_ = hw_selected;
  sw_fallback_applied_ = sw_fallback_applied;
}

void MetricsPublisher::publishHeartbeat() {
  RCLCPP_DEBUG(node_.get_logger(), "metrics heartbeat");
  publishSnapshot();
}

void MetricsPublisher::recordFrameIn(uint64_t now_ns, uint64_t source_timestamp_ns) {
  frames_in_.fetch_add(1, std::memory_order_relaxed);

  if (source_timestamp_ns == 0 || now_ns < source_timestamp_ns) {
    return;
  }

  const uint64_t latency_ns = now_ns - source_timestamp_ns;
  constexpr double kNsToMs = 1.0 / 1000000.0;
  const double sample_ms = static_cast<double>(latency_ns) * kNsToMs;

  if (std::isfinite(sample_ms) && sample_ms >= 0.0 && sample_ms < 10000.0) {
    constexpr double alpha = 0.2;
    const double previous = latency_estimate_ms_.load(std::memory_order_relaxed);
    const double next =
        previous <= 0.0 ? sample_ms : ((1.0 - alpha) * previous + alpha * sample_ms);
    latency_estimate_ms_.store(next, std::memory_order_relaxed);
  }
}

void MetricsPublisher::recordFrameOut() {
  frames_out_.fetch_add(1, std::memory_order_relaxed);
}

void MetricsPublisher::recordFrameDroppedBackpressure() {
  frames_dropped_backpressure_.fetch_add(1, std::memory_order_relaxed);
}

void MetricsPublisher::recordFrameDroppedMalformed() {
  frames_dropped_malformed_.fetch_add(1, std::memory_order_relaxed);
}

void MetricsPublisher::recordFrameDroppedThrottle() {
  frames_dropped_throttle_.fetch_add(1, std::memory_order_relaxed);
}

void MetricsPublisher::recordPushLatency(double latency_ms) {
  if (!std::isfinite(latency_ms) || latency_ms < 0.0) {
    return;
  }

  constexpr double alpha = 0.2;
  const double previous = push_latency_estimate_ms_.load(std::memory_order_relaxed);
  const double next =
      previous <= 0.0 ? latency_ms : ((1.0 - alpha) * previous + alpha * latency_ms);
  push_latency_estimate_ms_.store(next, std::memory_order_relaxed);

  double observed_max = push_latency_max_ms_.load(std::memory_order_relaxed);
  while (latency_ms > observed_max && !push_latency_max_ms_.compare_exchange_weak(
                                          observed_max, latency_ms, std::memory_order_relaxed)) {
  }
}

void MetricsPublisher::recordReconnect() {
  reconnect_count_.fetch_add(1, std::memory_order_relaxed);
}

void MetricsPublisher::updateRuntimeState(const std::string& runtime_state) {
  runtime_state_ = runtime_state;
}

void MetricsPublisher::publishEvent(const std::string& severity, const std::string& code,
                                    const std::string& message) {
  ros2_gst_video_bridge_msgs::msg::RuntimeEvent event;
  event.stamp = node_.now();
  event.session_id = session_id_;
  event.stream_id = stream_id_;
  event.severity = severity;
  event.code = code;
  event.message = message;
  event.state = runtime_state_;
  event.codec_name = codec_name_;
  event.encoder_name = encoder_name_;
  event.transport_kind = transport_kind_;
  event.sink_uri = sink_uri_;
  event.adaptation_level = adaptation_level_;
  event.software_fallback_applied = sw_fallback_applied_;
  event_pub_->publish(event);
}

ros2_gst_video_bridge_msgs::msg::RuntimeStatus
MetricsPublisher::buildStatusSnapshot(uint64_t now_ns, double elapsed_s) {
  ros2_gst_video_bridge_msgs::msg::RuntimeStatus status;
  status.stamp = node_.now();
  status.session_id = session_id_;
  status.stream_id = stream_id_;
  status.state = runtime_state_;
  status.source_topic = source_topic_;
  status.codec_name = codec_name_;
  status.encoder_name = encoder_name_;
  status.transport_kind = transport_kind_;
  status.sink_uri = sink_uri_;

  const uint64_t in_count = frames_in_.load(std::memory_order_relaxed);
  const uint64_t out_count = frames_out_.load(std::memory_order_relaxed);
  const uint64_t throttle_drops = frames_dropped_throttle_.load(std::memory_order_relaxed);
  const uint64_t malformed_drops = frames_dropped_malformed_.load(std::memory_order_relaxed);
  const uint64_t backpressure_drops = frames_dropped_backpressure_.load(std::memory_order_relaxed);
  const uint64_t drop_count = throttle_drops + malformed_drops + backpressure_drops;
  const uint64_t reconnects = reconnect_count_.load(std::memory_order_relaxed);

  status.fps_in = static_cast<float>(static_cast<double>(in_count - last_frames_in_) / elapsed_s);
  status.fps_out =
      static_cast<float>(static_cast<double>(out_count - last_frames_out_) / elapsed_s);
  status.dropped_total = drop_count;
  status.dropped_since_last = drop_count - last_frames_dropped_;
  status.dropped_throttle_total = throttle_drops;
  status.dropped_malformed_total = malformed_drops;
  status.dropped_backpressure_total = backpressure_drops;
  status.reconnect_count = reconnects;
  status.latency_estimate_ms =
      static_cast<float>(latency_estimate_ms_.load(std::memory_order_relaxed));
  status.push_latency_estimate_ms =
      static_cast<float>(push_latency_estimate_ms_.load(std::memory_order_relaxed));
  status.push_latency_max_ms =
      static_cast<float>(push_latency_max_ms_.load(std::memory_order_relaxed));
  status.max_fps_effective = max_fps_effective_;
  status.bitrate_kbps_effective = bitrate_kbps_effective_;
  status.gop_effective = gop_effective_;
  status.auto_codec_requested = auto_codec_requested_;
  status.hardware_encoder_selected = hw_encoder_selected_;
  status.software_fallback_applied = sw_fallback_applied_;
  status.adaptation_profile = adaptation_profile_;
  status.adaptation_level = adaptation_level_;

  (void)now_ns;
  return status;
}

std::string
MetricsPublisher::buildLegacyPayload(const ros2_gst_video_bridge_msgs::msg::RuntimeStatus& status,
                                     uint64_t drops_since_last) {
  std::ostringstream payload;
  payload << "state=" << status.state << " fps_in=" << status.fps_in
          << " fps_out=" << status.fps_out << " dropped_total=" << status.dropped_total
          << " dropped_since_last=" << drops_since_last
          << " dropped_throttle_total=" << status.dropped_throttle_total
          << " dropped_malformed_total=" << status.dropped_malformed_total
          << " dropped_backpressure_total=" << status.dropped_backpressure_total
          << " reconnect_count=" << status.reconnect_count
          << " latency_estimate_ms=" << status.latency_estimate_ms
          << " push_latency_estimate_ms=" << status.push_latency_estimate_ms
          << " push_latency_max_ms=" << status.push_latency_max_ms;
  return payload.str();
}

void MetricsPublisher::publishSnapshot() {
  const uint64_t now_ns = static_cast<uint64_t>(node_.now().nanoseconds());
  const uint64_t elapsed_ns = std::max<uint64_t>(1, now_ns - last_publish_time_ns_);
  const double elapsed_s = static_cast<double>(elapsed_ns) / 1000000000.0;

  const auto status = buildStatusSnapshot(now_ns, elapsed_s);
  status_pub_->publish(status);

  std_msgs::msg::String msg;
  msg.data = buildLegacyPayload(status, status.dropped_since_last);
  metrics_pub_->publish(msg);

  last_frames_in_ = frames_in_.load(std::memory_order_relaxed);
  last_frames_out_ = frames_out_.load(std::memory_order_relaxed);
  last_frames_dropped_ = status.dropped_total;
  last_publish_time_ns_ = now_ns;
}

} // namespace ros2_gst_video_bridge
