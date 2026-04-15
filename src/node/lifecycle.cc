// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/gst_video_bridge_node.hpp"

namespace ros2_gst_video_bridge {

GstVideoBridgeNode::~GstVideoBridgeNode() {
  if (stream_engine_) {
    stream_engine_->stop();
  }
}

bool GstVideoBridgeNode::hasImmediateExit() const {
  return immediate_exit_code_ >= 0;
}

int GstVideoBridgeNode::immediateExitCode() const {
  return immediate_exit_code_;
}

} // namespace ros2_gst_video_bridge
