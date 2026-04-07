// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/runtime/stream_engine.hpp"

namespace ros2_gst_video_bridge
{

StreamEngine::StreamEngine(std::string pipeline)
: pipeline_(std::move(pipeline))
{
}

void StreamEngine::start()
{
  running_ = true;
}

void StreamEngine::stop()
{
  running_ = false;
}

bool StreamEngine::isRunning() const
{
  return running_;
}

const std::string & StreamEngine::pipeline() const
{
  return pipeline_;
}

}  // namespace ros2_gst_video_bridge
