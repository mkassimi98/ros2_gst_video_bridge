// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/runtime/metrics_publisher.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace ros2_gst_video_bridge
{

MetricsPublisher::MetricsPublisher(rclcpp::Node & node)
: node_(node)
{
  metrics_pub_ = node_.create_publisher<std_msgs::msg::String>(
    "~/runtime_metrics", rclcpp::SystemDefaultsQoS());

  last_publish_time_ns_ = static_cast<uint64_t>(node_.now().nanoseconds());
  publish_timer_ = node_.create_wall_timer(
    std::chrono::seconds(1), [this]() { publishSnapshot(); });
}

void MetricsPublisher::publishHeartbeat()
{
  RCLCPP_DEBUG(node_.get_logger(), "metrics heartbeat");
  publishSnapshot();
}

void MetricsPublisher::recordFrameIn(uint64_t now_ns, uint64_t source_timestamp_ns)
{
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
    const double next = previous <= 0.0 ? sample_ms : ((1.0 - alpha) * previous + alpha * sample_ms);
    latency_estimate_ms_.store(next, std::memory_order_relaxed);
  }
}

void MetricsPublisher::recordFrameOut()
{
  frames_out_.fetch_add(1, std::memory_order_relaxed);
}

void MetricsPublisher::recordFrameDropped()
{
  frames_dropped_.fetch_add(1, std::memory_order_relaxed);
}

void MetricsPublisher::recordReconnect()
{
  reconnect_count_.fetch_add(1, std::memory_order_relaxed);
}

void MetricsPublisher::updateRuntimeState(const std::string & runtime_state)
{
  runtime_state_ = runtime_state;
}

void MetricsPublisher::publishSnapshot()
{
  const uint64_t now_ns = static_cast<uint64_t>(node_.now().nanoseconds());
  const uint64_t elapsed_ns = std::max<uint64_t>(1, now_ns - last_publish_time_ns_);
  const double elapsed_s = static_cast<double>(elapsed_ns) / 1000000000.0;

  const uint64_t in_count = frames_in_.load(std::memory_order_relaxed);
  const uint64_t out_count = frames_out_.load(std::memory_order_relaxed);
  const uint64_t drop_count = frames_dropped_.load(std::memory_order_relaxed);
  const uint64_t reconnects = reconnect_count_.load(std::memory_order_relaxed);

  const double fps_in = static_cast<double>(in_count - last_frames_in_) / elapsed_s;
  const double fps_out = static_cast<double>(out_count - last_frames_out_) / elapsed_s;
  const uint64_t drops_since_last = drop_count - last_frames_dropped_;

  std::ostringstream payload;
  payload << "state=" << runtime_state_
          << " fps_in=" << fps_in
          << " fps_out=" << fps_out
          << " dropped_total=" << drop_count
          << " dropped_since_last=" << drops_since_last
          << " reconnect_count=" << reconnects
          << " latency_estimate_ms=" << latency_estimate_ms_.load(std::memory_order_relaxed);

  std_msgs::msg::String msg;
  msg.data = payload.str();
  metrics_pub_->publish(msg);

  last_frames_in_ = in_count;
  last_frames_out_ = out_count;
  last_frames_dropped_ = drop_count;
  last_publish_time_ns_ = now_ns;
}

}  // namespace ros2_gst_video_bridge
