// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__DETAIL__STRING_UTILS_HPP_
#define ROS2_GST_VIDEO_BRIDGE__DETAIL__STRING_UTILS_HPP_

// Internal-only header: not installed, not part of the public API.

#include <string>
#include <string_view>

namespace ros2_gst_video_bridge {
namespace detail {

inline bool startsWith(const std::string& value, std::string_view prefix)
{
    return value.rfind(std::string(prefix), 0) == 0;
}

// Parse "codec -> element_name [tag]" entries produced by CapabilityProbe.
inline std::string extractElementName(const std::string& implementation)
{
    const auto arrow = implementation.find(" -> ");
    if (arrow == std::string::npos) {
        return "";
    }

    const auto name_start = arrow + 4;
    const auto bracket    = implementation.find(" [", name_start);
    if (bracket == std::string::npos || bracket <= name_start) {
        return "";
    }

    return implementation.substr(name_start, bracket - name_start);
}

} // namespace detail
} // namespace ros2_gst_video_bridge

#endif // ROS2_GST_VIDEO_BRIDGE__DETAIL__STRING_UTILS_HPP_
