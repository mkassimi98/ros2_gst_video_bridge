// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__DETAIL__ENCODER_SELECTION_HPP_
#define ROS2_GST_VIDEO_BRIDGE__DETAIL__ENCODER_SELECTION_HPP_

// Internal-only header: not installed, not part of the public API.
// Contains pure free-function logic for selecting a software encoder from the
// capability probe implementation list, so the logic can be unit-tested
// independently of the node.

#include "ros2_gst_video_bridge/detail/string_utils.hpp"

#include <string>
#include <vector>

namespace ros2_gst_video_bridge {
namespace detail {

// Scan `encoder_implementations` for the first entry whose codec prefix matches
// `codec_name` and whose tag is "[sw]".  Returns the element name (e.g.
// "x264enc") or an empty string when none is found.
//
// Each entry is expected to have the form: "codec_name -> element_name [tag]"
inline std::string
selectSoftwareEncoderForCodec(const std::string& codec_name,
                              const std::vector<std::string>& encoder_implementations) {
  const std::string prefix = codec_name + " -> ";
  for (const auto& impl : encoder_implementations) {
    if (!startsWith(impl, prefix)) {
      continue;
    }
    if (impl.find("[sw]") == std::string::npos) {
      continue;
    }
    const std::string encoder = extractElementName(impl);
    if (!encoder.empty()) {
      return encoder;
    }
  }
  return "";
}

} // namespace detail
} // namespace ros2_gst_video_bridge

#endif // ROS2_GST_VIDEO_BRIDGE__DETAIL__ENCODER_SELECTION_HPP_
