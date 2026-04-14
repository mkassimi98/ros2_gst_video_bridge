// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/core/pipeline_builder.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace ros2_gst_video_bridge {

namespace {

std::string toLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

std::pair<std::string, std::string> parseUdpHostPort(const std::string& sink_uri) {
  constexpr const char* kPrefix = "udp://";
  if (sink_uri.rfind(kPrefix, 0) != 0) {
    return {"127.0.0.1", "5000"};
  }

  const auto host_port = sink_uri.substr(std::char_traits<char>::length(kPrefix));
  const auto separator = host_port.find(':');
  if (separator == std::string::npos) {
    return {host_port, "5000"};
  }

  const auto host = host_port.substr(0, separator);
  const auto port = host_port.substr(separator + 1);
  if (host.empty() || port.empty()) {
    return {"127.0.0.1", "5000"};
  }

  return {host, port};
}

bool isOneOf(const std::string& value, const std::initializer_list<const char*>& items) {
  for (const auto* item : items) {
    if (value == item) {
      return true;
    }
  }
  return false;
}

std::string buildEncoderChain(const GstBridgeConfig& config, const std::string& codec,
                              const std::string& selected_encoder) {
  const std::string encoder =
      selected_encoder.empty()
          ? (codec == "h265" ? "x265enc" : (codec == "mjpeg" ? "jpegenc" : "x264enc"))
          : selected_encoder;

  std::ostringstream chain;

  const bool is_nv_v4l2_h264 = encoder == "nvv4l2h264enc";
  const bool is_nv_v4l2_h265 = encoder == "nvv4l2h265enc";
  const bool is_nv_v4l2 = is_nv_v4l2_h264 || is_nv_v4l2_h265;

  if (is_nv_v4l2) {
    // nvv4l2 encoders on Jetson require NVMM input; convert from appsrc CPU memory first.
    chain << "videoconvert ! "
          << "video/x-raw,format=I420 ! "
          << "nvvidconv ! "
          << "video/x-raw(memory:NVMM),format=NV12 ! " << encoder << " "
          << "bitrate=" << (config.codec.bitrate_kbps * 1000) << " "
          << "control-rate=" << (config.codec.rate_control == "vbr" ? 0 : 1) << " "
          << "iframeinterval=" << config.codec.gop << " "
          << "insert-sps-pps=true "
          << "insert-vui=true "
          << "maxperf-enable=true "
          << "! ";

    if (is_nv_v4l2_h265 || codec == "h265") {
      chain << "h265parse config-interval=1 ! ";
    } else {
      chain << "h264parse config-interval=1 ! ";
    }

    return chain.str();
  }

  if (encoder == "x264enc") {
    chain << "videoconvert ! "
          << "video/x-raw,format=I420 ! "
          << "x264enc "
          << "tune=" << config.codec.tune << " "
          << "bitrate=" << config.codec.bitrate_kbps << " "
          << "key-int-max=" << config.codec.gop << " "
          << "speed-preset=ultrafast "
          << "! h264parse config-interval=1 ! ";
    return chain.str();
  }

  if (encoder == "x265enc") {
    chain << "videoconvert ! "
          << "video/x-raw,format=I420 ! "
          << "x265enc "
          << "tune=" << config.codec.tune << " "
          << "bitrate=" << config.codec.bitrate_kbps << " "
          << "key-int-max=" << config.codec.gop << " "
          << "! h265parse config-interval=1 ! ";
    return chain.str();
  }

  if (encoder == "nvjpegenc") {
    chain << "videoconvert ! "
          << "video/x-raw,format=I420 ! "
          << "nvvidconv ! "
          << "video/x-raw(memory:NVMM),format=I420 ! "
          << "nvjpegenc ! ";
    return chain.str();
  }

  if (isOneOf(encoder, {"jpegenc", "vaapijpegenc", "v4l2jpegenc"})) {
    chain << "videoconvert ! "
          << "video/x-raw,format=I420 ! " << encoder << " ! ";
    return chain.str();
  }

  // Conservative generic branch for unknown encoders.
  chain << "videoconvert ! " << encoder << " ! ";
  return chain.str();
}

void appendPayloadOrMux(std::ostringstream& pipeline, const std::string& transport,
                        const std::string& codec) {
  if (transport == "udp" || transport == "rtsp") {
    if (codec == "h265") {
      pipeline << "rtph265pay config-interval=1 pt=96 ! ";
    } else if (codec == "mjpeg") {
      pipeline << "rtpjpegpay pt=26 ! ";
    } else {
      pipeline << "rtph264pay config-interval=1 pt=96 ! ";
    }
    return;
  }

  if (codec == "mjpeg") {
    pipeline << "matroskamux streamable=true ! ";
  } else {
    pipeline << "mpegtsmux alignment=7 ! ";
  }
}

} // namespace

std::string PipelineBuilder::build(const GstBridgeConfig& config) {
  if (!config.gst.pipeline_override.empty()) {
    return config.gst.pipeline_override;
  }

  const auto codec = toLower(config.codec.name);
  const auto encoder = toLower(config.codec.encoder);
  const auto transport = toLower(config.transport.kind);

  std::ostringstream pipeline;
  pipeline << "appsrc name=bridge_appsrc is-live=true format=time do-timestamp=true "
           << "block=true ! "
           << "queue leaky=downstream max-size-buffers=2 max-size-time=0 max-size-bytes=0 ! "
           << buildEncoderChain(config, codec, encoder);

  appendPayloadOrMux(pipeline, transport, codec);

  if (transport == "udp") {
    const auto [host, port] = parseUdpHostPort(config.transport.sink_uri);
    pipeline << "udpsink host=" << host << " port=" << port << " sync=false async=false";
  } else if (transport == "rtsp") {
    pipeline << "rtspclientsink location=" << config.transport.sink_uri << " protocols=tcp";
  } else if (transport == "file") {
    pipeline << "filesink location=" << config.transport.sink_uri;
  } else {
    pipeline << "srtsink "
             << "uri=" << config.transport.sink_uri << " "
             << "latency=" << config.transport.latency_ms << " "
             << "wait-for-connection=false "
             << "sync=false async=false";
  }

  return pipeline.str();
}

} // namespace ros2_gst_video_bridge
