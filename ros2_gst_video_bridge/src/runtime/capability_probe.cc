// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/runtime/capability_probe.hpp"

#include <array>
#include <cstdio>
#include <fstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace ros2_gst_video_bridge {

namespace {

#if defined(__aarch64__)
constexpr const char* kBuildArch = "aarch64";
#elif defined(__arm__)
constexpr const char* kBuildArch = "arm";
#elif defined(__x86_64__)
constexpr const char* kBuildArch = "x86_64";
#elif defined(__i386__)
constexpr const char* kBuildArch = "x86";
#else
constexpr const char* kBuildArch = "unknown";
#endif

std::string runCommand(const std::string& command)
{
    std::array<char, 256> buffer{};
    std::string           result;

    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr) {
        return result;
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result += buffer.data();
    }

    pclose(pipe);
    return result;
}

bool commandHasAnyOutput(const std::string& command)
{
    return !runCommand(command).empty();
}

std::string trim(std::string value)
{
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r' || value.back() == ' ')) {
        value.pop_back();
    }

    size_t start = 0;
    while (start < value.size() && value[start] == ' ') {
        ++start;
    }

    return value.substr(start);
}

std::string readFirstCpuModelLine()
{
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
        return "unknown";
    }

    std::string line;
    while (std::getline(cpuinfo, line)) {
        const auto model_pos    = line.find("model name");
        const auto hardware_pos = line.find("Hardware");
        if (model_pos == 0 || hardware_pos == 0) {
            const auto sep = line.find(':');
            if (sep != std::string::npos) {
                return trim(line.substr(sep + 1));
            }
        }
    }

    return "unknown";
}

bool fileExists(const char* path)
{
    std::ifstream f(path);
    return f.good();
}

std::vector<std::string> filterAvailableCodecElements(
        const std::vector<std::tuple<std::string, std::string, std::string>>& candidates)
{
    std::vector<std::string> available;
    for (const auto& candidate : candidates) {
        const std::string& element = std::get<0>(candidate);
        const std::string& codec   = std::get<1>(candidate);
        const std::string& accel   = std::get<2>(candidate);
        const std::string  command = "gst-inspect-1.0 " + element + " 2>/dev/null";
        if (commandHasAnyOutput(command)) {
            available.push_back(codec + " -> " + element + " [" + accel + "]");
        }
    }
    return available;
}

std::vector<std::string> filterAvailableElements(const std::vector<std::pair<std::string, std::string>>& candidates)
{
    std::vector<std::string> available;
    for (const auto& candidate : candidates) {
        const std::string command = "gst-inspect-1.0 " + candidate.first + " 2>/dev/null";
        if (commandHasAnyOutput(command)) {
            available.push_back(candidate.second + " (" + candidate.first + ")");
        }
    }
    return available;
}

} // namespace

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
    const auto               core_plugins = filterAvailableElements(
            {{"appsrc", "appsrc"}, {"videoconvert", "videoconvert"}, {"videoscale", "videoscale"}, {"queue", "queue"}});
    plugins.insert(plugins.end(), core_plugins.begin(), core_plugins.end());

    const auto transport_plugins = filterAvailableElements(
            {{"srtsink", "SRT sink"},
             {"rtspclientsink", "RTSP client sink"},
             {"udpsink", "UDP sink"},
             {"filesink", "File sink"}});
    plugins.insert(plugins.end(), transport_plugins.begin(), transport_plugins.end());

    const auto codec_plugins = filterAvailableElements(
            {{"av1enc", "AV1 encoder"},
             {"x265enc", "H265 encoder"},
             {"x264enc", "H264 encoder"},
             {"jpegenc", "MJPEG encoder"}});
    plugins.insert(plugins.end(), codec_plugins.begin(), codec_plugins.end());

    return plugins;
}

std::vector<std::string> CapabilityProbe::detectEncoders() const
{
    const auto implementations = detectEncoderImplementations();
    if (implementations.empty()) {
        return {"av1", "h265", "h264", "mjpeg"};
    }

    bool has_av1   = false;
    bool has_h265  = false;
    bool has_h264  = false;
    bool has_mjpeg = false;
    for (const auto& impl : implementations) {
        if (impl.rfind("av1", 0) == 0) {
            has_av1 = true;
        } else if (impl.rfind("h265", 0) == 0) {
            has_h265 = true;
        } else if (impl.rfind("h264", 0) == 0) {
            has_h264 = true;
        } else if (impl.rfind("mjpeg", 0) == 0) {
            has_mjpeg = true;
        }
    }

    std::vector<std::string> encoders;
    if (has_av1) {
        encoders.push_back("av1");
    }
    if (has_h265) {
        encoders.push_back("h265");
    }
    if (has_h264) {
        encoders.push_back("h264");
    }
    if (has_mjpeg) {
        encoders.push_back("mjpeg");
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

std::vector<std::string> CapabilityProbe::detectHostProfile() const
{
    std::vector<std::string> profile;
    profile.push_back(std::string("build_arch=") + kBuildArch);

    const std::string runtime_arch = trim(runCommand("uname -m 2>/dev/null"));
    const std::string runtime_os   = trim(runCommand("uname -s 2>/dev/null"));
    const std::string kernel       = trim(runCommand("uname -r 2>/dev/null"));
    profile.push_back("runtime_arch=" + (runtime_arch.empty() ? std::string("unknown") : runtime_arch));
    profile.push_back("runtime_os=" + (runtime_os.empty() ? std::string("unknown") : runtime_os));
    profile.push_back("kernel=" + (kernel.empty() ? std::string("unknown") : kernel));
    profile.push_back("cpu_model=" + readFirstCpuModelLine());

    const bool jetson_hint = fileExists("/etc/nv_tegra_release") || fileExists("/dev/nvhost-ctrl")
                          || fileExists("/proc/driver/nvidia/version");
    profile.push_back(std::string("platform_hint=") + (jetson_hint ? "jetson_or_nvidia" : "generic"));

    return profile;
}

std::vector<std::string> CapabilityProbe::detectEncoderImplementations() const
{
    if (!hasGstInspect()) {
        return {"av1 -> av1enc [sw]", "h265 -> x265enc [sw]", "h264 -> x264enc [sw]", "mjpeg -> jpegenc [sw]"};
    }

    return filterAvailableCodecElements(
            {{"av1enc", "av1", "sw"},
             {"rav1enc", "av1", "sw"},
             {"svtav1enc", "av1", "sw"},
             {"avenc_libaom_av1", "av1", "sw"},
             {"avenc_libsvtav1", "av1", "sw"},
             {"x265enc", "h265", "sw"},
             {"x264enc", "h264", "sw"},
             {"openh264enc", "h264", "sw"},
             {"jpegenc", "mjpeg", "sw"},
             {"nvv4l2av1enc", "av1", "hw:nvidia-v4l2"},
             {"nvv4l2h264enc", "h264", "hw:nvidia-v4l2"},
             {"nvv4l2h265enc", "h265", "hw:nvidia-v4l2"},
             {"nvav1enc", "av1", "hw:nvidia"},
             {"nvh264enc", "h264", "hw:nvidia"},
             {"nvh265enc", "h265", "hw:nvidia"},
             {"nvjpegenc", "mjpeg", "hw:nvidia"},
             {"vaav1enc", "av1", "hw:vaapi"},
             {"vaapiav1enc", "av1", "hw:vaapi"},
             {"qsvav1enc", "av1", "hw:qsv"},
             {"vaapih264enc", "h264", "hw:vaapi"},
             {"vaapih265enc", "h265", "hw:vaapi"},
             {"vaapijpegenc", "mjpeg", "hw:vaapi"},
             {"v4l2av1enc", "av1", "hw:v4l2"},
             {"v4l2h264enc", "h264", "hw:v4l2"},
             {"v4l2h265enc", "h265", "hw:v4l2"},
             {"v4l2jpegenc", "mjpeg", "hw:v4l2"},
             {"omxh264enc", "h264", "hw:omx"},
             {"omxh265enc", "h265", "hw:omx"}});
}

} // namespace ros2_gst_video_bridge
