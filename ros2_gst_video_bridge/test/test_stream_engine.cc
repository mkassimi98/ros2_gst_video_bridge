#include "ros2_gst_video_bridge/runtime/stream_engine.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

TEST(StreamEngineTest, PushesSyntheticFrameToFakesink) {
  ros2_gst_video_bridge::StreamEngine engine(
      "appsrc name=bridge_appsrc is-live=true format=time block=false max-buffers=2 "
      "max-bytes=0 max-time=0 leaky-type=upstream ! "
      "queue leaky=downstream max-size-buffers=2 max-size-time=0 max-size-bytes=0 ! "
      "videoconvert ! fakesink sync=false async=false");

  ASSERT_TRUE(engine.start()) << engine.lastError();

  constexpr int width = 32;
  constexpr int height = 24;
  std::vector<uint8_t> frame(static_cast<size_t>(width * height * 3), 127);

  EXPECT_TRUE(engine.pushFrame(frame.data(), frame.size(), width, height, "BGR", 1000000))
      << engine.lastError();

  engine.stop();
  EXPECT_FALSE(engine.isRunning());
}

} // namespace
