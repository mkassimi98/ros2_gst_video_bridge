// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/core/pipeline_builder.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace ros2_gst_video_bridge
{

namespace
{

std::string toLower(std::string value)
{
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

std::pair<std::string, std::string> parseUdpHostPort(const std::string & sink_uri)
{
  constexpr const char * kPrefix = "udp://";
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

}  // namespace

std::string PipelineBuilder::build(const GstBridgeConfig & config)
{
  if (!config.gst.pipeline_override.empty()) {
    return config.gst.pipeline_override;
  }

  const auto codec = toLower(config.codec.name);
  const auto transport = toLower(config.transport.kind);

  std::ostringstream pipeline;
  pipeline << "appsrc name=bridge_appsrc is-live=true format=time do-timestamp=true ! "
           << "videoconvert ! ";

  if (codec == "h265") {
    pipeline << "x265enc "
             << "tune=" << config.codec.tune << " "
             << "bitrate=" << config.codec.bitrate_kbps << " "
             << "key-int-max=" << config.codec.gop << " ! ";
  } else if (codec == "mjpeg") {
    pipeline << "jpegenc ! ";
  } else {
    pipeline << "x264enc "
             << "tune=" << config.codec.tune << " "
             << "bitrate=" << config.codec.bitrate_kbps << " "
             << "key-int-max=" << config.codec.gop << " "
             << "speed-preset=ultrafast "
             << "! ";
  }

  if (transport == "udp") {
    pipeline << "rtph264pay config-interval=1 pt=96 ! ";
  } else {
      pipeline << "mpegtsmux alignment=7 ! ";
  }

  if (transport == "udp") {
    const auto [host, port] = parseUdpHostPort(config.transport.sink_uri);
    pipeline << "udpsink host=" << host << " port=" << port << " sync=false async=false";
  } else if (transport == "rtsp") {
    pipeline << "rtspclientsink location=" << config.transport.sink_uri;
  } else if (transport == "file") {
    pipeline << "filesink location=" << config.transport.sink_uri;
  } else {
    pipeline << "srtsink "
             << "uri=" << config.transport.sink_uri << " "
             << "latency=" << config.transport.latency_ms << " "
             << "wait-for-connection=false";
  }

  return pipeline.str();
}

}  // namespace ros2_gst_video_bridge
