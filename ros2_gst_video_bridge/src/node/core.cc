// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/gst_video_bridge_node.hpp"

#include "ros2_gst_video_bridge/core/config_loader.hpp"
#include "ros2_gst_video_bridge/core/pipeline_builder.hpp"
#include "ros2_gst_video_bridge/detail/preflight_policy.hpp"
#include "ros2_gst_video_bridge/detail/string_utils.hpp"

#include <utility>
#include <vector>
namespace ros2_gst_video_bridge {
namespace {

std::string valueOf(const std::vector<std::string>& entries, const std::string& key) {
  for (const auto& entry : entries) {
    const auto sep = entry.find('=');
    if (sep == std::string::npos) {
      continue;
    }
    if (entry.substr(0, sep) == key) {
      return entry.substr(sep + 1);
    }
  }
  return "";
}

std::string detectMachineProfile(const std::string& configured_machine,
                                 const std::vector<std::string>& host_profile) {
  if (configured_machine != "generic") {
    return configured_machine;
  }

  const std::string platform_hint = valueOf(host_profile, "platform_hint");
  const std::string runtime_arch = valueOf(host_profile, "runtime_arch");

  if (platform_hint == "jetson_or_nvidia") {
    return "jetson";
  }

  if (runtime_arch == "x86_64" || runtime_arch == "amd64" || runtime_arch == "x86" ||
      runtime_arch == "i386") {
    return "x86";
  }

  if (runtime_arch == "aarch64" || detail::startsWith(runtime_arch, "arm")) {
    return "raspi";
  }

  return "generic";
}

std::string extractElementName(const std::string& implementation) {
  return detail::extractElementName(implementation);
}

bool isHardwareImplementation(const std::string& implementation) {
  return implementation.find("[hw:") != std::string::npos;
}

std::string defaultSinkUriForTransport(const std::string& transport) {
  if (transport == "udp") {
    return "udp://127.0.0.1:5000";
  }
  if (transport == "rtsp") {
    return "rtsp://127.0.0.1:8554/stream";
  }
  if (transport == "file") {
    return "/tmp/ros2_gst_video_bridge.ts";
  }
  return "srt://127.0.0.1:9000?mode=listener";
}

bool sinkUriMatchesTransport(const std::string& transport, const std::string& sink_uri) {
  if (transport == "srt") {
    return detail::startsWith(sink_uri, "srt://");
  }
  if (transport == "udp") {
    return detail::startsWith(sink_uri, "udp://");
  }
  if (transport == "rtsp") {
    return detail::startsWith(sink_uri, "rtsp://");
  }
  if (transport == "file") {
    return !detail::startsWith(sink_uri, "srt://") && !detail::startsWith(sink_uri, "udp://") &&
           !detail::startsWith(sink_uri, "rtsp://");
  }
  return false;
}

int implementationScore(const std::string& machine, const std::string& impl) {
  const bool is_sw = impl.find("[sw]") != std::string::npos;
  const bool is_nv_v4l2 = impl.find("[hw:nvidia-v4l2]") != std::string::npos;
  const bool is_nv = impl.find("[hw:nvidia]") != std::string::npos;
  const bool is_vaapi = impl.find("[hw:vaapi]") != std::string::npos;
  const bool is_qsv = impl.find("[hw:qsv]") != std::string::npos;
  const bool is_v4l2 = impl.find("[hw:v4l2]") != std::string::npos;
  const bool is_omx = impl.find("[hw:omx]") != std::string::npos;

  const int hw_base = 200;
  const int sw_base = 100;

  if (machine == "jetson") {
    if (is_nv_v4l2) {
      return hw_base + 30;
    }
    if (is_nv) {
      return hw_base + 25;
    }
    if (is_v4l2 || is_omx || is_vaapi) {
      return hw_base + 10;
    }
    return is_sw ? sw_base : 0;
  }

  if (machine == "x86") {
    if (is_vaapi) {
      return hw_base + 30;
    }
    if (is_qsv) {
      return hw_base + 28;
    }
    if (is_v4l2) {
      return hw_base + 20;
    }
    if (is_nv_v4l2 || is_nv) {
      return hw_base + 15;
    }
    return is_sw ? sw_base : 0;
  }

  if (machine == "raspi") {
    if (is_v4l2) {
      return hw_base + 30;
    }
    if (is_omx) {
      return hw_base + 25;
    }
    if (is_nv_v4l2 || is_nv || is_vaapi) {
      return hw_base + 10;
    }
    return is_sw ? sw_base : 0;
  }

  if (is_vaapi) {
    return hw_base + 25;
  }
  if (is_qsv) {
    return hw_base + 22;
  }
  if (is_v4l2) {
    return hw_base + 20;
  }
  if (is_nv_v4l2 || is_nv) {
    return hw_base + 15;
  }
  if (is_omx) {
    return hw_base + 10;
  }
  return is_sw ? sw_base : 0;
}

std::string selectAutoCodec(const std::string& machine,
                            const std::vector<std::string>& implementations, std::string& reason) {
  const std::vector<std::pair<std::string, int>> codec_priority{
      {"av1", 4}, {"h265", 3}, {"h264", 2}, {"mjpeg", 1}};

  for (const auto& codec_item : codec_priority) {
    const std::string& codec = codec_item.first;
    int best_score_for_codec = -1;
    std::string best_impl_for_codec;

    for (const auto& impl : implementations) {
      if (!detail::startsWith(impl, codec + " -> ")) {
        continue;
      }

      const int score = implementationScore(machine, impl);
      if (score > best_score_for_codec) {
        best_score_for_codec = score;
        best_impl_for_codec = impl;
      }
    }

    if (!best_impl_for_codec.empty()) {
      reason = best_impl_for_codec;
      return codec;
    }
  }

  reason = "fallback";
  return "h265";
}

} // namespace
GstVideoBridgeNode::GstVideoBridgeNode(const rclcpp::NodeOptions& options)
    : Node("gst_video_bridge", options) {
  session_id_ = buildSessionId();
  stream_id_ = this->declare_parameter<std::string>("runtime.stream_id", "default");

  hw_fallback_failure_threshold_ = this->declare_parameter<int>("runtime.hw_fallback_failures", 3);
  if (hw_fallback_failure_threshold_ < 1) {
    hw_fallback_failure_threshold_ = 1;
  }

  adaptation_enabled_ = this->declare_parameter<bool>("runtime.adaptation.enabled", true);
  adaptation_interval_ms_ = this->declare_parameter<int>("runtime.adaptation.interval_ms", 2000);
  adaptation_cooldown_ms_ = this->declare_parameter<int>("runtime.adaptation.cooldown_ms", 5000);
  adaptation_profile_ =
      this->declare_parameter<std::string>("runtime.adaptation.profile", "balanced");

  if (!isValidAdaptationProfile(adaptation_profile_)) {
    RCLCPP_WARN(this->get_logger(),
                "invalid runtime.adaptation.profile='%s', forcing to 'balanced'",
                adaptation_profile_.c_str());
    adaptation_profile_ = "balanced";
  }

  if (adaptation_interval_ms_ < 500) {
    adaptation_interval_ms_ = 500;
  }
  if (adaptation_cooldown_ms_ < adaptation_interval_ms_) {
    adaptation_cooldown_ms_ = adaptation_interval_ms_;
  }

  config_ = ConfigLoader::loadFromNode(*this);
  base_bitrate_kbps_ = config_.codec.bitrate_kbps;
  base_gop_ = config_.codec.gop;
  base_max_fps_ = config_.runtime.max_fps;

  topic_introspector_ = std::make_unique<TopicIntrospector>(*this);
  capability_probe_ = std::make_unique<CapabilityProbe>();

  const auto host_profile = capability_probe_->detectHostProfile();
  const std::string machine_for_auto = detectMachineProfile(config_.profile.machine, host_profile);

  if (machine_for_auto != config_.profile.machine) {
    RCLCPP_INFO(this->get_logger(),
                "profile.machine '%s' auto-resolved to '%s' from host detection",
                config_.profile.machine.c_str(), machine_for_auto.c_str());
  }

  auto_codec_requested_ = config_.codec.name == "auto";
  encoder_implementations_ = capability_probe_->detectEncoderImplementations();

  const auto available_sinks = capability_probe_->detectSinks();
  const auto transport_preflight =
      detail::resolveTransportForAvailableSinks(config_.transport.kind, available_sinks);
  if (transport_preflight.fallback_applied) {
    RCLCPP_WARN(this->get_logger(), "%s", transport_preflight.reason.c_str());
    publishRuntimeEvent("warning", "TRANSPORT_FALLBACK", transport_preflight.reason);
    config_.transport.kind = transport_preflight.effective_transport;
  }
  if (!sinkUriMatchesTransport(config_.transport.kind, config_.transport.sink_uri)) {
    const std::string old_uri = config_.transport.sink_uri;
    config_.transport.sink_uri = defaultSinkUriForTransport(config_.transport.kind);
    RCLCPP_WARN(this->get_logger(),
                "transport.sink_uri '%s' incompatible with transport.kind='%s'. Using '%s'.",
                old_uri.c_str(), config_.transport.kind.c_str(),
                config_.transport.sink_uri.c_str());
  }

  if (config_.codec.name == "auto") {
    std::string selection_reason;
    const std::string selected_codec =
        selectAutoCodec(machine_for_auto, encoder_implementations_, selection_reason);
    config_.codec.name = selected_codec;
    config_.codec.encoder = extractElementName(selection_reason);
    hw_encoder_selected_ = isHardwareImplementation(selection_reason);

    RCLCPP_INFO(
        this->get_logger(),
        "codec.name=auto resolved to '%s' with encoder '%s' using '%s' (effective machine: %s)",
        config_.codec.name.c_str(), config_.codec.encoder.c_str(), selection_reason.c_str(),
        machine_for_auto.c_str());
  } else {
    hw_encoder_selected_ = false;

    const auto encoder_preflight = detail::resolveEncoderOverride(
        config_.codec.name, config_.codec.encoder, encoder_implementations_);
    if (encoder_preflight.fallback_applied) {
      RCLCPP_WARN(this->get_logger(), "%s", encoder_preflight.reason.c_str());
      publishRuntimeEvent("warning", "ENCODER_PREFLIGHT_FALLBACK", encoder_preflight.reason);
      config_.codec.encoder = encoder_preflight.effective_encoder;
    } else if (!encoder_preflight.effective_encoder.empty()) {
      config_.codec.encoder = encoder_preflight.effective_encoder;
    }
  }

  effective_pipeline_ = PipelineBuilder::build(config_);

  if (config_.runtime.print_effective_config) {
    RCLCPP_INFO(this->get_logger(), "Effective runtime configuration:\n%s",
                ConfigLoader::toDebugString(config_).c_str());
  }

  if (!validateConfiguration()) {
    RCLCPP_ERROR(this->get_logger(), "Configuration validation failed. Node will shutdown.");
    publishRuntimeEvent("error", "CONFIG_INVALID", "Configuration validation failed");
    immediate_exit_code_ = 1;
    return;
  }

  if (handleRuntimeMode()) {
    immediate_exit_code_ = 0;
    return;
  }

  initializeModules();
  initializeSubscriptions();
  initializeHealthMonitoring();

  if (immediate_exit_code_ >= 0) {
    return;
  }

  logConfiguration();
  logRuntimeCapabilities();

  RCLCPP_INFO(this->get_logger(),
              "Streaming node ready. Subscribed to %s and forwarding frames to GStreamer appsrc.",
              config_.source.input_topic.c_str());

  if (stream_engine_ && stream_engine_->isRunning()) {
    setRuntimeState(RuntimeState::Streaming, "pipeline running and subscriptions active");
  }
}

} // namespace ros2_gst_video_bridge
