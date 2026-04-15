// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/gst_video_bridge_node.hpp"

#include "ros2_gst_video_bridge/core/config_loader.hpp"
#include "ros2_gst_video_bridge/core/pipeline_builder.hpp"

#include <string_view>
#include <utility>
#include <vector>
namespace ros2_gst_video_bridge {
namespace {

bool startsWith(const std::string& value, const std::string_view prefix) {
  return value.rfind(std::string(prefix), 0) == 0;
}

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

  if (runtime_arch == "aarch64" || startsWith(runtime_arch, "arm")) {
    return "raspi";
  }

  return "generic";
}

std::string extractElementName(const std::string& implementation) {
  const auto arrow = implementation.find(" -> ");
  if (arrow == std::string::npos) {
    return "";
  }

  const auto name_start = arrow + 4;
  const auto bracket = implementation.find(" [", name_start);
  if (bracket == std::string::npos || bracket <= name_start) {
    return "";
  }

  return implementation.substr(name_start, bracket - name_start);
}

bool isHardwareImplementation(const std::string& implementation) {
  return implementation.find("[hw:") != std::string::npos;
}

int implementationScore(const std::string& machine, const std::string& impl) {
  const bool is_sw = impl.find("[sw]") != std::string::npos;
  const bool is_nv_v4l2 = impl.find("[hw:nvidia-v4l2]") != std::string::npos;
  const bool is_nv = impl.find("[hw:nvidia]") != std::string::npos;
  const bool is_vaapi = impl.find("[hw:vaapi]") != std::string::npos;
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
      {"h264", 3}, {"h265", 2}, {"mjpeg", 1}};

  int best_score = -1;
  int best_priority = -1;
  std::string best_codec = "h264";
  std::string best_impl = "fallback";

  for (const auto& codec_item : codec_priority) {
    const std::string& codec = codec_item.first;
    const int priority = codec_item.second;

    for (const auto& impl : implementations) {
      if (!startsWith(impl, codec + " -> ")) {
        continue;
      }

      const int score = implementationScore(machine, impl);
      if (score > best_score || (score == best_score && priority > best_priority)) {
        best_score = score;
        best_priority = priority;
        best_codec = codec;
        best_impl = impl;
      }
    }
  }

  reason = best_impl;
  return best_codec;
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

  if (config_.codec.name == "auto") {
    encoder_implementations_ = capability_probe_->detectEncoderImplementations();
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
