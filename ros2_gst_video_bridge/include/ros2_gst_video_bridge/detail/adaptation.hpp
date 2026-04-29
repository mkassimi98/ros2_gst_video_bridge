// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__DETAIL__ADAPTATION_HPP_
#define ROS2_GST_VIDEO_BRIDGE__DETAIL__ADAPTATION_HPP_

// Internal-only header: not installed, not part of the public API.
// Contains the pure numeric policy for adaptation level scaling so that
// the formulas can be unit-tested independently of the node.

#include <cstdint>
#include <string>

namespace ros2_gst_video_bridge {
namespace detail {

struct AdaptationScales
{
    double fps_scale{1.0};
    double bitrate_scale{1.0};
};

// Returns the fps and bitrate scale factors for the given profile and level (0–2).
// Level 0 always returns {1.0, 1.0} (no degradation).
inline AdaptationScales computeAdaptationScales(const std::string& profile, uint8_t level)
{
    const uint8_t clamped = level > 2 ? 2 : level;

    if (profile == "conservative") {
        if (clamped == 0) {
            return {1.0, 1.0};
        }
        if (clamped == 1) {
            return {0.85, 0.75};
        }
        return {0.70, 0.55};
    }

    if (profile == "aggressive") {
        if (clamped == 0) {
            return {1.0, 1.0};
        }
        if (clamped == 1) {
            return {0.75, 0.60};
        }
        return {0.55, 0.40};
    }

    // "balanced" (default for any unrecognised profile)
    if (clamped == 0) {
        return {1.0, 1.0};
    }
    if (clamped == 1) {
        return {0.80, 0.65};
    }
    return {0.60, 0.45};
}

} // namespace detail
} // namespace ros2_gst_video_bridge

#endif // ROS2_GST_VIDEO_BRIDGE__DETAIL__ADAPTATION_HPP_
