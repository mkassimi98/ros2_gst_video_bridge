// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__RUNTIME__IMAGE_FORMAT_HPP_
#define ROS2_GST_VIDEO_BRIDGE__RUNTIME__IMAGE_FORMAT_HPP_

#include <cstddef>
#include <sensor_msgs/msg/image.hpp>
#include <string>

namespace ros2_gst_video_bridge {

struct ImageShapeValidation
{
    bool        valid{false};
    std::string error;
};

bool                 resolveGstFormat(const std::string& encoding, std::string& gst_format);
std::string          resolveGstBayerFormat(const std::string& encoding);
bool                 isBayerEncoding(const std::string& encoding);
size_t               bytesPerPixelForEncoding(const std::string& encoding);
ImageShapeValidation validateImageShape(const sensor_msgs::msg::Image& msg);

} // namespace ros2_gst_video_bridge

#endif // ROS2_GST_VIDEO_BRIDGE__RUNTIME__IMAGE_FORMAT_HPP_
