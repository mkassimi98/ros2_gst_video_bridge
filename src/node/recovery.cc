// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/gst_video_bridge_node.hpp"

#include "ros2_gst_video_bridge/core/pipeline_builder.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>

namespace ros2_gst_video_bridge {

void GstVideoBridgeNode::runHealthCheck() {
  if (!stream_engine_ || runtime_state_ == RuntimeState::Failed) {
    return;
  }

  if (reconnect_requested_) {
    (void)tryReconnect();
    return;
  }

  evaluateAdaptationPolicy();

  const auto engine_error = stream_engine_->lastError();
  if (!engine_error.empty()) {
    scheduleReconnect(engine_error);
  }
}

void GstVideoBridgeNode::evaluateAdaptationPolicy() {
  if (!adaptation_enabled_) {
    return;
  }

  const uint64_t now_ns = static_cast<uint64_t>(this->now().nanoseconds());
  if (last_adaptation_ns_ != 0) {
    const uint64_t since_last_ms = (now_ns - last_adaptation_ns_) / 1000000ULL;
    if (since_last_ms < static_cast<uint64_t>(adaptation_interval_ms_)) {
      return;
    }
  }

  uint8_t target_level = adaptation_level_;
  if (runtime_state_ == RuntimeState::Degraded || runtime_state_ == RuntimeState::Reconnecting) {
    target_level = std::min<uint8_t>(2, static_cast<uint8_t>(adaptation_level_ + 1));
  } else if (runtime_state_ == RuntimeState::Streaming && reconnect_count_ == 0 &&
             adaptation_level_ > 0) {
    target_level = static_cast<uint8_t>(adaptation_level_ - 1);
  }

  if (target_level != adaptation_level_) {
    applyAdaptationLevel(target_level, "runtime state transition");
    last_adaptation_ns_ = now_ns;
  }
}

void GstVideoBridgeNode::applyAdaptationLevel(uint8_t level, const std::string& reason) {
  adaptation_level_ = std::min<uint8_t>(2, level);

  double fps_scale = 1.0;
  double bitrate_scale = 1.0;

  if (adaptation_profile_ == "conservative") {
    fps_scale = adaptation_level_ == 0 ? 1.0 : (adaptation_level_ == 1 ? 0.85 : 0.70);
    bitrate_scale = adaptation_level_ == 0 ? 1.0 : (adaptation_level_ == 1 ? 0.75 : 0.55);
  } else if (adaptation_profile_ == "aggressive") {
    fps_scale = adaptation_level_ == 0 ? 1.0 : (adaptation_level_ == 1 ? 0.75 : 0.55);
    bitrate_scale = adaptation_level_ == 0 ? 1.0 : (adaptation_level_ == 1 ? 0.60 : 0.40);
  } else {
    fps_scale = adaptation_level_ == 0 ? 1.0 : (adaptation_level_ == 1 ? 0.80 : 0.60);
    bitrate_scale = adaptation_level_ == 0 ? 1.0 : (adaptation_level_ == 1 ? 0.65 : 0.45);
  }

  const int next_bitrate = std::max(400, static_cast<int>(base_bitrate_kbps_ * bitrate_scale));
  const int next_gop =
      std::max(10, static_cast<int>(base_gop_ * (adaptation_level_ == 0 ? 1.0 : 0.8)));
  const double next_fps = std::max(5.0, base_max_fps_ * fps_scale);

  bool changed = false;
  if (next_bitrate != config_.codec.bitrate_kbps) {
    config_.codec.bitrate_kbps = next_bitrate;
    changed = true;
  }
  if (next_gop != config_.codec.gop) {
    config_.codec.gop = next_gop;
    changed = true;
  }
  if (std::abs(next_fps - config_.runtime.max_fps) > 0.01) {
    config_.runtime.max_fps = next_fps;
    changed = true;
  }

  if (!changed) {
    return;
  }

  if (metrics_publisher_) {
    metrics_publisher_->setAdaptationState(adaptation_profile_, adaptation_level_);
    metrics_publisher_->setEffectiveEncoding(static_cast<uint32_t>(config_.codec.bitrate_kbps),
                                             static_cast<uint32_t>(config_.codec.gop),
                                             static_cast<float>(config_.runtime.max_fps));
  }

  std::ostringstream message;
  message << "adaptation_level=" << static_cast<int>(adaptation_level_) << " reason=" << reason
          << " bitrate_kbps=" << config_.codec.bitrate_kbps << " gop=" << config_.codec.gop
          << " max_fps=" << config_.runtime.max_fps;

  publishRuntimeEvent("warning", "ADAPTATION_APPLIED", message.str());
  (void)requestPipelineReconfigure("adaptation policy updated encoding knobs");
}

bool GstVideoBridgeNode::requestPipelineReconfigure(const std::string& reason) {
  effective_pipeline_ = PipelineBuilder::build(config_);
  pipeline_reconfigure_requested_ = true;

  if (metrics_publisher_) {
    metrics_publisher_->setSessionMetadata(session_id_, stream_id_, config_.source.input_topic,
                                           config_.codec.name, config_.codec.encoder,
                                           config_.transport.kind, config_.transport.sink_uri);
  }

  if (!reconnect_requested_) {
    reconnect_requested_ = true;
    setRuntimeState(RuntimeState::Degraded, "pipeline reconfigure requested");
  }

  publishRuntimeEvent("info", "PIPELINE_RECONFIGURE", reason);
  return true;
}

void GstVideoBridgeNode::scheduleReconnect(const std::string& reason) {
  if (runtime_state_ == RuntimeState::Failed) {
    return;
  }

  publishFirstFailureSnapshot(reason);

  if (!config_.transport.reconnect_enabled) {
    setRuntimeState(RuntimeState::Failed, "reconnect disabled: " + reason);
    return;
  }

  if (!reconnect_requested_) {
    reconnect_requested_ = true;
    setRuntimeState(RuntimeState::Degraded, reason);
    publishRuntimeEvent("warning", "PIPELINE_DEGRADED", reason);
    (void)requestSoftwareFallback(reason);
  }
}

bool GstVideoBridgeNode::tryReconnect() {
  if (!stream_engine_) {
    return false;
  }

  const auto now = std::chrono::steady_clock::now();
  const auto reconnect_interval =
      std::chrono::milliseconds(config_.transport.reconnect_interval_ms);
  if (last_reconnect_attempt_.time_since_epoch().count() != 0 &&
      (now - last_reconnect_attempt_) < reconnect_interval) {
    return false;
  }

  if (config_.transport.reconnect_max_attempts > 0 &&
      reconnect_attempts_ >= config_.transport.reconnect_max_attempts) {
    setRuntimeState(RuntimeState::Failed, "reconnect attempts exhausted");
    reconnect_requested_ = false;
    return false;
  }

  last_reconnect_attempt_ = now;
  ++reconnect_attempts_;
  setRuntimeState(RuntimeState::Reconnecting, "attempt " + std::to_string(reconnect_attempts_));

  ++reconnect_count_;
  stream_engine_->stop();

  if (sw_fallback_requested_ || pipeline_reconfigure_requested_) {
    stream_engine_ = std::make_unique<StreamEngine>(effective_pipeline_);
    sw_fallback_requested_ = false;
    pipeline_reconfigure_requested_ = false;
  }

  const bool started = stream_engine_->start();
  if (!started) {
    setRuntimeState(RuntimeState::Degraded, stream_engine_->lastError());
    publishRuntimeEvent("error", "RECONNECT_FAILED", stream_engine_->lastError());
    publishFirstFailureSnapshot(stream_engine_->lastError());
    (void)requestSoftwareFallback(stream_engine_->lastError());
    return false;
  }

  if (metrics_publisher_) {
    metrics_publisher_->recordReconnect();
  }

  reconnect_requested_ = false;
  reconnect_attempts_ = 0;
  consecutive_stream_failures_ = 0;
  last_frame_steady_time_ = std::chrono::steady_clock::time_point{};
  setRuntimeState(RuntimeState::Streaming, "reconnected successfully");
  return true;
}

} // namespace ros2_gst_video_bridge
