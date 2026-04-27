// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/core/config_loader.hpp"
#include "ros2_gst_video_bridge/detail/string_utils.hpp"

#include <set>
#include <sstream>

namespace ros2_gst_video_bridge {

namespace {

bool inSet(const std::string& value, const std::set<std::string>& allowed) {
  return allowed.find(value) != allowed.end();
}

std::string forceSrtListenerMode(const std::string& sink_uri) {
  if (sink_uri.rfind("srt://", 0) != 0) {
    return sink_uri;
  }

  std::string normalized = sink_uri;
  const auto mode_pos = normalized.find("mode=");
  if (mode_pos == std::string::npos) {
    const char separator = normalized.find('?') == std::string::npos ? '?' : '&';
    normalized += separator;
    normalized += "mode=listener";
    return normalized;
  }

  const auto value_begin = mode_pos + 5;
  const auto value_end = normalized.find('&', value_begin);
  normalized.replace(value_begin,
                     value_end == std::string::npos ? std::string::npos : value_end - value_begin,
                     "listener");
  return normalized;
}

} // namespace

GstBridgeConfig ConfigLoader::loadFromNode(rclcpp::Node& node) {
  GstBridgeConfig config;

  config.profile.machine =
      node.declare_parameter<std::string>("profile.machine", config.profile.machine);
  config.profile.stream =
      node.declare_parameter<std::string>("profile.stream", config.profile.stream);

  applyMachineProfileDefaults(config);
  applyStreamProfileDefaults(config);

  config.source.input_topic =
      node.declare_parameter<std::string>("input_topic", config.source.input_topic);

  config.transport.kind =
      node.declare_parameter<std::string>("gst.transport", config.transport.kind);
  config.transport.kind =
      node.declare_parameter<std::string>("transport.kind", config.transport.kind);
  config.transport.sink_uri =
      node.declare_parameter<std::string>("gst.sink_uri", config.transport.sink_uri);
  config.transport.sink_uri =
      node.declare_parameter<std::string>("transport.sink_uri", config.transport.sink_uri);
  if (config.transport.kind == "srt") {
    config.transport.sink_uri = forceSrtListenerMode(config.transport.sink_uri);
  }
  config.transport.latency_ms =
      node.declare_parameter<int>("gst.latency_ms", config.transport.latency_ms);
  config.transport.latency_ms =
      node.declare_parameter<int>("transport.latency_ms", config.transport.latency_ms);
  config.transport.reconnect_enabled = node.declare_parameter<bool>(
      "transport.reconnect.enabled", config.transport.reconnect_enabled);
  config.transport.reconnect_interval_ms = node.declare_parameter<int>(
      "transport.reconnect.interval_ms", config.transport.reconnect_interval_ms);
  config.transport.reconnect_max_attempts = node.declare_parameter<int>(
      "transport.reconnect.max_attempts", config.transport.reconnect_max_attempts);

  config.codec.name = node.declare_parameter<std::string>("gst.codec", config.codec.name);
  config.codec.name = node.declare_parameter<std::string>("codec.name", config.codec.name);
  config.codec.encoder = node.declare_parameter<std::string>("codec.encoder", config.codec.encoder);
  config.codec.profile = node.declare_parameter<std::string>("gst.profile", config.codec.profile);
  config.codec.profile = node.declare_parameter<std::string>("codec.profile", config.codec.profile);
  config.codec.tune = node.declare_parameter<std::string>("codec.tune", config.codec.tune);
  config.codec.rate_control =
      node.declare_parameter<std::string>("codec.rate_control", config.codec.rate_control);
  config.codec.bitrate_kbps =
      node.declare_parameter<int>("gst.bitrate_kbps", config.codec.bitrate_kbps);
  config.codec.bitrate_kbps =
      node.declare_parameter<int>("codec.bitrate_kbps", config.codec.bitrate_kbps);
  config.codec.gop = node.declare_parameter<int>("codec.gop", config.codec.gop);

  config.runtime.max_fps = node.declare_parameter<double>("max_fps", config.runtime.max_fps);
  config.runtime.use_wall_clock_timestamps = node.declare_parameter<bool>(
      "use_wall_clock_timestamps", config.runtime.use_wall_clock_timestamps);
  config.runtime.mode = node.declare_parameter<std::string>("runtime.mode", config.runtime.mode);
  config.runtime.print_effective_config = node.declare_parameter<bool>(
      "runtime.print_effective_config", config.runtime.print_effective_config);

  config.gst.pipeline_override =
      node.declare_parameter<std::string>("gst.pipeline_override", config.gst.pipeline_override);

  return config;
}

void ConfigLoader::applyMachineProfileDefaults(GstBridgeConfig& config) {
  if (config.profile.machine == "jetson") {
    config.codec.name = "auto";
    config.codec.tune = "zerolatency";
    config.codec.bitrate_kbps = 2500;
    config.codec.gop = 30;
    config.transport.latency_ms = 70;
  } else if (config.profile.machine == "x86") {
    config.codec.name = "auto";
    config.codec.bitrate_kbps = 4000;
    config.codec.gop = 60;
    config.transport.latency_ms = 90;
  } else if (config.profile.machine == "raspi") {
    config.codec.name = "auto";
    config.codec.profile = "main";
    config.codec.tune = "zerolatency";
    config.codec.bitrate_kbps = 1800;
    config.codec.gop = 30;
    config.runtime.max_fps = 25.0;
    config.transport.latency_ms = 80;
  }
}

void ConfigLoader::applyStreamProfileDefaults(GstBridgeConfig& config) {
  if (config.profile.stream == "low_bandwidth") {
    config.runtime.max_fps = 15.0;
    config.codec.bitrate_kbps = 1200;
    config.codec.gop = 30;
    config.transport.kind = "srt";
    config.transport.latency_ms = 120;
  } else if (config.profile.stream == "low_latency") {
    config.runtime.max_fps = 30.0;
    config.codec.bitrate_kbps = 2000;
    config.codec.gop = 30;
    config.codec.tune = "zerolatency";
    config.transport.kind = "srt";
    config.transport.sink_uri = "srt://127.0.0.1:9000?mode=listener";
    config.transport.latency_ms = 60;
  } else if (config.profile.stream == "high_quality") {
    config.runtime.max_fps = 30.0;
    config.codec.bitrate_kbps = 6000;
    config.codec.gop = 60;
    config.transport.kind = "srt";
    config.transport.sink_uri = "srt://127.0.0.1:9000?mode=listener";
    config.transport.latency_ms = 100;
  } else if (config.profile.stream == "monitoring_udp") {
    config.transport.kind = "udp";
    config.transport.sink_uri = "udp://127.0.0.1:5000";
    config.codec.bitrate_kbps = 2500;
  }
}

std::vector<std::string> ConfigLoader::validate(const GstBridgeConfig& config) {
  std::vector<std::string> errors;

  const std::set<std::string> valid_machines{"generic", "jetson", "x86", "raspi"};
  const std::set<std::string> valid_stream_profiles{"default", "low_latency", "low_bandwidth",
                                                    "high_quality", "monitoring_udp"};
  const std::set<std::string> valid_modes{"stream", "list_topics", "list_capabilities",
                                          "validate_config", "discover"};
  const std::set<std::string> valid_transports{"srt", "rtsp", "udp", "file"};
  const std::set<std::string> valid_codecs{"auto", "av1", "h265", "h264", "mjpeg"};

  if (!inSet(config.profile.machine, valid_machines)) {
    errors.emplace_back(
        "profile.machine must be one of [generic, jetson, x86, raspi]. Current value: '" +
        config.profile.machine + "'.");
  }

  if (!inSet(config.profile.stream, valid_stream_profiles)) {
    errors.emplace_back(
        "profile.stream must be one of [default, low_latency, low_bandwidth, high_quality, "
        "monitoring_udp]. Current value: '" +
        config.profile.stream + "'.");
  }

  if (!inSet(config.runtime.mode, valid_modes)) {
    errors.emplace_back(
        "runtime.mode must be one of [stream, list_topics, list_capabilities, validate_config, "
        "discover]. Current value: '" +
        config.runtime.mode + "'.");
  }

  if (config.source.input_topic.empty() || config.source.input_topic.front() != '/') {
    errors.emplace_back("input_topic must be an absolute ROS topic starting with '/'. Example: "
                        "'/camera/image_raw'.");
  }

  if (!inSet(config.transport.kind, valid_transports)) {
    errors.emplace_back("transport.kind must be one of [srt, rtsp, udp, file]. Current value: '" +
                        config.transport.kind + "'.");
  }

  if (config.gst.pipeline_override.empty() && config.transport.sink_uri.empty()) {
    errors.emplace_back(
        "transport.sink_uri cannot be empty unless gst.pipeline_override is provided.");
  }

  if (!config.gst.pipeline_override.empty()) {
    // Custom pipeline override takes full responsibility for compatibility checks.
  } else if (config.transport.kind == "srt" &&
             !detail::startsWith(config.transport.sink_uri, "srt://")) {
    errors.emplace_back("transport.sink_uri must start with 'srt://' when transport.kind=srt.");
  } else if (config.transport.kind == "rtsp" &&
             !detail::startsWith(config.transport.sink_uri, "rtsp://")) {
    errors.emplace_back("transport.sink_uri must start with 'rtsp://' when transport.kind=rtsp.");
  } else if (config.transport.kind == "udp" &&
             !detail::startsWith(config.transport.sink_uri, "udp://")) {
    errors.emplace_back("transport.sink_uri must start with 'udp://' when transport.kind=udp.");
  } else if (config.transport.kind == "file" &&
             detail::startsWith(config.transport.sink_uri, "udp://")) {
    errors.emplace_back(
        "transport.sink_uri for transport.kind=file must be a file path, not UDP URI.");
  }

  if (config.transport.latency_ms < 0) {
    errors.emplace_back("transport.latency_ms must be >= 0.");
  }

  if (config.transport.reconnect_enabled && config.transport.reconnect_interval_ms <= 0) {
    errors.emplace_back("transport.reconnect.interval_ms must be > 0 when reconnect is enabled.");
  }

  if (config.transport.reconnect_max_attempts < 0) {
    errors.emplace_back("transport.reconnect.max_attempts must be >= 0 (0 means unlimited).");
  }

  if (!inSet(config.codec.name, valid_codecs)) {
    errors.emplace_back(
        "codec.name must be one of [auto, av1, h265, h264, mjpeg]. Current value: '" +
        config.codec.name + "'.");
  }

  if (config.codec.bitrate_kbps <= 0) {
    errors.emplace_back("codec.bitrate_kbps must be > 0.");
  }

  if (config.codec.gop <= 0) {
    errors.emplace_back("codec.gop must be > 0.");
  }

  if (config.runtime.max_fps <= 0.0) {
    errors.emplace_back("max_fps must be > 0.");
  }

  return errors;
}

std::string ConfigLoader::toDebugString(const GstBridgeConfig& config) {
  std::ostringstream out;
  out << "profile.machine=" << config.profile.machine << '\n';
  out << "profile.stream=" << config.profile.stream << '\n';
  out << "input_topic=" << config.source.input_topic << '\n';
  out << "transport.kind=" << config.transport.kind << '\n';
  out << "transport.sink_uri=" << config.transport.sink_uri << '\n';
  out << "transport.latency_ms=" << config.transport.latency_ms << '\n';
  out << "transport.reconnect.enabled=" << (config.transport.reconnect_enabled ? "true" : "false")
      << '\n';
  out << "transport.reconnect.interval_ms=" << config.transport.reconnect_interval_ms << '\n';
  out << "transport.reconnect.max_attempts=" << config.transport.reconnect_max_attempts << '\n';
  out << "codec.name=" << config.codec.name << '\n';
  out << "codec.encoder=" << config.codec.encoder << '\n';
  out << "codec.profile=" << config.codec.profile << '\n';
  out << "codec.tune=" << config.codec.tune << '\n';
  out << "codec.rate_control=" << config.codec.rate_control << '\n';
  out << "codec.bitrate_kbps=" << config.codec.bitrate_kbps << '\n';
  out << "codec.gop=" << config.codec.gop << '\n';
  out << "max_fps=" << config.runtime.max_fps << '\n';
  out << "use_wall_clock_timestamps="
      << (config.runtime.use_wall_clock_timestamps ? "true" : "false") << '\n';
  out << "runtime.mode=" << config.runtime.mode << '\n';
  out << "runtime.print_effective_config="
      << (config.runtime.print_effective_config ? "true" : "false") << '\n';
  out << "gst.pipeline_override=" << config.gst.pipeline_override;
  return out.str();
}

} // namespace ros2_gst_video_bridge
