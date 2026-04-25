// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__CORE__PIPELINE_BUILDER_HPP_
#define ROS2_GST_VIDEO_BRIDGE__CORE__PIPELINE_BUILDER_HPP_

#include "ros2_gst_video_bridge/core/gst_bridge_config.hpp"

#include <string>

namespace ros2_gst_video_bridge {

class PipelineBuilder {
public:
  static std::string build(const GstBridgeConfig& config);
};

} // namespace ros2_gst_video_bridge

#endif // ROS2_GST_VIDEO_BRIDGE__CORE__PIPELINE_BUILDER_HPP_
