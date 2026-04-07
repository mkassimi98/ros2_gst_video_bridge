// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__RUNTIME__STREAM_ENGINE_HPP_
#define ROS2_GST_VIDEO_BRIDGE__RUNTIME__STREAM_ENGINE_HPP_

#include <string>

namespace ros2_gst_video_bridge
{

class StreamEngine
{
public:
  explicit StreamEngine(std::string pipeline);

  void start();
  void stop();
  bool isRunning() const;
  const std::string & pipeline() const;

private:
  std::string pipeline_;
  bool running_{false};
};

}  // namespace ros2_gst_video_bridge

#endif  // ROS2_GST_VIDEO_BRIDGE__RUNTIME__STREAM_ENGINE_HPP_
