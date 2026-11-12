#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "ros2_gst_video_bridge/detail/image_buffer.hpp"
#include "ros2_gst_video_bridge/runtime/stream_engine.hpp"

namespace {

sensor_msgs::msg::Image::UniquePtr makeImage() {
  auto image = std::make_unique<sensor_msgs::msg::Image>();
  image->width = 32;
  image->height = 24;
  image->step = 100; // BGR with four padding bytes per row.
  image->encoding = "bgr8";
  image->data.resize(image->step * image->height, 127);
  return image;
}

TEST(StreamEngineTest, WrapsOriginalPixelsAndKeepsMemoryAliveBeyondBuffer) {
  gst_init(nullptr, nullptr);
  auto image = makeImage();
  const auto *pixels = image->data.data();
  GstBuffer *buffer = ros2_gst_video_bridge::detail::wrapImageBuffer(std::move(image), "BGR");
  ASSERT_NE(buffer, nullptr);
  EXPECT_EQ(image, nullptr);
  auto *meta = gst_buffer_get_video_meta(buffer);
  ASSERT_NE(meta, nullptr);
  EXPECT_EQ(meta->stride[0], 100);

  // Downstream may retain GstMemory independently of its original GstBuffer.
  GstMemory *memory = gst_memory_ref(gst_buffer_peek_memory(buffer, 0));
  EXPECT_TRUE(GST_MEMORY_IS_READONLY(memory));
  gst_buffer_unref(buffer);
  GstMapInfo map{};
  ASSERT_TRUE(gst_memory_map(memory, &map, GST_MAP_READ));
  EXPECT_EQ(map.data, pixels);
  EXPECT_EQ(map.size, 2400U);
  EXPECT_EQ(map.data[map.size - 1], 127);
  gst_memory_unmap(memory, &map);
  gst_memory_unref(memory);
}

TEST(StreamEngineTest, RejectsEmptyAndTruncatedOwnedImages) {
  gst_init(nullptr, nullptr);
  EXPECT_EQ(ros2_gst_video_bridge::detail::wrapImageBuffer(nullptr, "BGR"), nullptr);
  auto image = makeImage();
  image->data.pop_back();
  EXPECT_EQ(ros2_gst_video_bridge::detail::wrapImageBuffer(std::move(image), "BGR"), nullptr);
}

TEST(StreamEngineTest, PushesOwnedImageAndRejectsWhenStopped) {
  ros2_gst_video_bridge::StreamEngine engine(
      "appsrc name=bridge_appsrc ! videoconvert ! fakesink sync=false async=false");
  EXPECT_FALSE(engine.pushFrame(makeImage(), "BGR", 1000000));
  ASSERT_TRUE(engine.start()) << engine.lastError();
  EXPECT_TRUE(engine.pushFrame(makeImage(), "BGR", 1000000)) << engine.lastError();
  engine.stop();
  EXPECT_FALSE(engine.pushFrame(makeImage(), "BGR", 2000000));
}

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

TEST(StreamEngineTest, AcceptsNonMonotonicSourceTimestamps) {
  ros2_gst_video_bridge::StreamEngine engine(
      "appsrc name=bridge_appsrc is-live=true format=time block=false max-buffers=2 "
      "max-bytes=0 max-time=0 leaky-type=upstream ! "
      "queue leaky=downstream max-size-buffers=2 max-size-time=0 max-size-bytes=0 ! "
      "videoconvert ! fakesink sync=false async=false");

  ASSERT_TRUE(engine.start()) << engine.lastError();

  constexpr int width = 32;
  constexpr int height = 24;
  std::vector<uint8_t> frame(static_cast<size_t>(width * height * 3), 127);

  EXPECT_TRUE(engine.pushFrame(frame.data(), frame.size(), width, height, "BGR", 5000000))
      << engine.lastError();
  EXPECT_TRUE(engine.pushFrame(frame.data(), frame.size(), width, height, "BGR", 1000000))
      << engine.lastError();
  EXPECT_TRUE(engine.lastError().empty());

  engine.stop();
}

} // namespace
