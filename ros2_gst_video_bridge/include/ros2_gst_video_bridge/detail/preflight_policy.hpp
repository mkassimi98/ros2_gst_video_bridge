// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__DETAIL__PREFLIGHT_POLICY_HPP_
#define ROS2_GST_VIDEO_BRIDGE__DETAIL__PREFLIGHT_POLICY_HPP_

#include <array>
#include <string>
#include <vector>

#include "ros2_gst_video_bridge/detail/encoder_selection.hpp"

namespace ros2_gst_video_bridge {
namespace detail {

struct TransportPreflightResult
{
    std::string effective_transport;
    bool        fallback_applied{false};
    std::string reason;
};

struct EncoderPreflightResult
{
    std::string effective_encoder;
    bool        fallback_applied{false};
    std::string reason;
};

inline bool hasTransport(const std::vector<std::string>& available_sinks, const std::string& transport)
{
    for (const auto& sink : available_sinks) {
        if (sink == transport) {
            return true;
        }
    }
    return false;
}

inline TransportPreflightResult resolveTransportForAvailableSinks(
        const std::string&              requested_transport,
        const std::vector<std::string>& available_sinks)
{
    if (available_sinks.empty() || hasTransport(available_sinks, requested_transport)) {
        return {requested_transport, false, "requested transport available"};
    }

    static constexpr std::array<const char*, 4> kFallbackOrder{{"srt", "udp", "file", "rtsp"}};
    for (const auto* candidate : kFallbackOrder) {
        if (hasTransport(available_sinks, candidate)) {
            return {candidate,
                    true,
                    "transport '" + requested_transport + "' unavailable, using '" + candidate + "' detected on host"};
        }
    }

    return {requested_transport, false, "no detected sink candidates; keeping requested transport"};
}

inline bool hasEncoderImplementationForCodec(
        const std::vector<std::string>& implementations,
        const std::string&              codec,
        const std::string&              encoder_element)
{
    for (const auto& impl : implementations) {
        if (!startsWith(impl, codec + " -> ")) {
            continue;
        }
        if (extractElementName(impl) == encoder_element) {
            return true;
        }
    }
    return false;
}

inline EncoderPreflightResult resolveEncoderOverride(
        const std::string&              codec,
        const std::string&              requested_encoder,
        const std::vector<std::string>& implementations)
{
    if (requested_encoder.empty()) {
        return {"", false, "no encoder override requested"};
    }

    if (hasEncoderImplementationForCodec(implementations, codec, requested_encoder)) {
        return {requested_encoder, false, "requested encoder override available"};
    }

    const std::string sw_encoder = selectSoftwareEncoderForCodec(codec, implementations);
    if (!sw_encoder.empty()) {
        return {sw_encoder,
                true,
                "encoder '" + requested_encoder + "' unavailable, using software fallback '" + sw_encoder + "'"};
    }

    return {"", true, "encoder override unavailable and no software fallback found"};
}

} // namespace detail
} // namespace ros2_gst_video_bridge

#endif // ROS2_GST_VIDEO_BRIDGE__DETAIL__PREFLIGHT_POLICY_HPP_
