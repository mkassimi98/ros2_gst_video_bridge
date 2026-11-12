#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

#include "ros2_gst_video_bridge/gst_video_bridge_node.hpp"

namespace {

struct Observation {
  std::mutex mutex;
  const uint8_t *pixels{nullptr};
  uint8_t value{0};
  bool received{false};
};

struct HandoffHook {
  guint signal;
  gulong hook;
  ~HandoffHook() { g_signal_remove_emission_hook(signal, hook); }
};

class ZeroCopyNodeTest : public ::testing::TestWithParam<bool> {
protected:
  static void SetUpTestSuite() {
    rclcpp::init(0, nullptr);
    gst_init(nullptr, nullptr);
  }
  static void TearDownTestSuite() { rclcpp::shutdown(); }
};

TEST_P(ZeroCopyNodeTest, ChecksPixelAddressFromPublisherThroughGstreamer) {
  // Observe the real sink without exposing StreamEngine's private pipeline.
  GstElement *probe = gst_element_factory_make("fakesink", nullptr);
  ASSERT_NE(probe, nullptr);
  const guint signal = g_signal_lookup("handoff", G_OBJECT_TYPE(probe));
  gst_object_unref(probe);
  ASSERT_NE(signal, 0U);

  auto observation = std::make_shared<Observation>();
  const gulong hook = g_signal_add_emission_hook(
      signal, 0,
      [](GSignalInvocationHint *, guint, const GValue *values, gpointer data) -> gboolean {
        auto &observed = **static_cast<std::shared_ptr<Observation> *>(data);
        auto *buffer = static_cast<GstBuffer *>(g_value_get_boxed(&values[1]));
        GstMapInfo map{};
        if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
          std::lock_guard<std::mutex> lock(observed.mutex);
          observed.pixels = map.data;
          observed.value = map.data[0];
          observed.received = true;
          gst_buffer_unmap(buffer, &map);
        }
        return TRUE;
      },
      new std::shared_ptr<Observation>(observation),
      [](gpointer data) { delete static_cast<std::shared_ptr<Observation> *>(data); });
  ASSERT_NE(hook, 0U);
  HandoffHook hook_guard{signal, hook};

  const bool zero_copy = GetParam();
  rclcpp::NodeOptions bridge_options;
  bridge_options.use_intra_process_comms(true);
  bridge_options.parameter_overrides({
      rclcpp::Parameter("input_topic", "/zero_copy_test/image"),
      rclcpp::Parameter("source.zero_copy", zero_copy),
      rclcpp::Parameter("runtime.adaptation.enabled", false),
      rclcpp::Parameter("runtime.print_effective_config", false),
      rclcpp::Parameter("gst.pipeline_override",
                        "appsrc name=bridge_appsrc ! fakesink signal-handoffs=true "
                        "sync=false async=false enable-last-sample=false"),
  });
  auto bridge = std::make_shared<ros2_gst_video_bridge::GstVideoBridgeNode>(bridge_options);
  ASSERT_FALSE(bridge->hasImmediateExit());
  rclcpp::NodeOptions producer_options;
  producer_options.use_intra_process_comms(true);
  auto producer = std::make_shared<rclcpp::Node>("zero_copy_test_producer", producer_options);
  auto publisher = producer->create_publisher<sensor_msgs::msg::Image>("/zero_copy_test/image",
                                                                       rclcpp::SensorDataQoS());

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(bridge);
  executor.add_node(producer);

  auto image = std::make_unique<sensor_msgs::msg::Image>();
  image->width = 32;
  image->height = 24;
  image->step = 96;
  image->encoding = "bgr8";
  image->data.resize(image->step * image->height, 127);
  const auto *published_pixels = image->data.data();
  publisher->publish(std::move(image));

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (std::chrono::steady_clock::now() < deadline) {
    executor.spin_some();
    {
      std::lock_guard<std::mutex> lock(observation->mutex);
      if (observation->received) {
        break;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  std::lock_guard<std::mutex> lock(observation->mutex);
  ASSERT_TRUE(observation->received);
  EXPECT_EQ(observation->value, 127);
  if (zero_copy) {
    EXPECT_EQ(observation->pixels, published_pixels);
  } else {
    EXPECT_NE(observation->pixels, published_pixels);
  }
}

INSTANTIATE_TEST_SUITE_P(InputModes, ZeroCopyNodeTest, ::testing::Bool());

} // namespace
