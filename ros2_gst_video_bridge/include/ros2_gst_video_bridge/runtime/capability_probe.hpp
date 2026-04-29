// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__RUNTIME__CAPABILITY_PROBE_HPP_
#define ROS2_GST_VIDEO_BRIDGE__RUNTIME__CAPABILITY_PROBE_HPP_

#include <string>
#include <vector>

namespace ros2_gst_video_bridge {

class CapabilityProbe
{
  public:
    bool                     hasGstInspect() const;
    std::vector<std::string> detectPlugins() const;
    std::vector<std::string> detectEncoders() const;
    std::vector<std::string> detectSinks() const;

    std::vector<std::string> detectHostProfile() const;
    std::vector<std::string> detectEncoderImplementations() const;

    std::vector<std::string> detectTransports() const;
    std::vector<std::string> detectCodecs() const;
};

} // namespace ros2_gst_video_bridge

#endif // ROS2_GST_VIDEO_BRIDGE__RUNTIME__CAPABILITY_PROBE_HPP_
