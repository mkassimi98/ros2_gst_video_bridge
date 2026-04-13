// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/runtime/capability_probe.hpp"

#include <array>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace ros2_gst_video_bridge
{

namespace
{

std::string runCommand(const std::string & command)
{
  std::array<char, 256> buffer{};
  std::string result;

  FILE * pipe = popen(command.c_str(), "r");
  if (pipe == nullptr) {
    return result;
  }

  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
    result += buffer.data();
  }

  pclose(pipe);
  return result;
}

bool commandHasAnyOutput(const std::string & command)
{
  return !runCommand(command).empty();
}

std::vector<std::string> filterAvailableElements(
  const std::vector<std::pair<std::string, std::string>> & candidates)
{
  std::vector<std::string> available;
  for (const auto & candidate : candidates) {
    const std::string command = "gst-inspect-1.0 " + candidate.first + " 2>/dev/null";
    if (commandHasAnyOutput(command)) {
      available.push_back(candidate.second + " (" + candidate.first + ")");
    }
  }
  return available;
}

}  // namespace

bool CapabilityProbe::hasGstInspect() const
{
  return commandHasAnyOutput("command -v gst-inspect-1.0");
}

std::vector<std::string> CapabilityProbe::detectPlugins() const
{
  if (!hasGstInspect()) {
    return {};
  }

  std::vector<std::string> plugins;
  const auto core_plugins = filterAvailableElements(
    { {"appsrc", "appsrc"},
      {"videoconvert", "videoconvert"},
      {"videoscale", "videoscale"},
      {"queue", "queue"} });
  plugins.insert(plugins.end(), core_plugins.begin(), core_plugins.end());

  const auto transport_plugins = filterAvailableElements(
    { {"srtsink", "SRT sink"},
      {"rtspclientsink", "RTSP client sink"},
      {"udpsink", "UDP sink"},
      {"filesink", "File sink"} });
  plugins.insert(plugins.end(), transport_plugins.begin(), transport_plugins.end());

  const auto codec_plugins = filterAvailableElements(
    { {"x264enc", "H264 encoder"},
      {"x265enc", "H265 encoder"},
      {"jpegenc", "MJPEG encoder"} });
  plugins.insert(plugins.end(), codec_plugins.begin(), codec_plugins.end());

  return plugins;
}

std::vector<std::string> CapabilityProbe::detectEncoders() const
{
  if (!hasGstInspect()) {
    return {"h264", "h265", "mjpeg"};
  }

  std::vector<std::string> encoders;

  if (commandHasAnyOutput("gst-inspect-1.0 x264enc 2>/dev/null")) {
    encoders.push_back("h264");
  }
  if (commandHasAnyOutput("gst-inspect-1.0 x265enc 2>/dev/null")) {
    encoders.push_back("h265");
  }
  if (commandHasAnyOutput("gst-inspect-1.0 jpegenc 2>/dev/null")) {
    encoders.push_back("mjpeg");
  }

  if (encoders.empty()) {
    return {"h264", "h265", "mjpeg"};
  }

  return encoders;
}

std::vector<std::string> CapabilityProbe::detectSinks() const
{
  if (!hasGstInspect()) {
    return {"srt", "rtsp", "udp", "file"};
  }

  std::vector<std::string> sinks;

  if (commandHasAnyOutput("gst-inspect-1.0 srtsink 2>/dev/null")) {
    sinks.push_back("srt");
  }
  if (commandHasAnyOutput("gst-inspect-1.0 rtspclientsink 2>/dev/null")) {
    sinks.push_back("rtsp");
  }
  if (commandHasAnyOutput("gst-inspect-1.0 udpsink 2>/dev/null")) {
    sinks.push_back("udp");
  }
  if (commandHasAnyOutput("gst-inspect-1.0 filesink 2>/dev/null")) {
    sinks.push_back("file");
  }

  if (sinks.empty()) {
    return {"srt", "rtsp", "udp", "file"};
  }

  return sinks;
}

std::vector<std::string> CapabilityProbe::detectTransports() const
{
  return detectSinks();
}

std::vector<std::string> CapabilityProbe::detectCodecs() const
{
  return detectEncoders();
}

}  // namespace ros2_gst_video_bridge
