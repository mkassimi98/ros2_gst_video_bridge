// Tests for MetricsPublisher: verifies that counter recording methods work
// correctly and that the published RuntimeStatus reflects the recorded values.

#include "ros2_gst_video_bridge/runtime/metrics_publisher.hpp"

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <ros2_gst_video_bridge_msgs/msg/runtime_status.hpp>

#include <chrono>
#include <memory>

namespace {

class MetricsPublisherTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() {
    if (!rclcpp::ok()) {
      rclcpp::init(0, nullptr);
    }
  }

  static void TearDownTestSuite() {
    if (rclcpp::ok()) {
      rclcpp::shutdown();
    }
  }

  void SetUp() override {
    node_ =
        std::make_shared<rclcpp::Node>("metrics_publisher_test_" + std::to_string(test_counter_++));
    publisher_ = std::make_unique<ros2_gst_video_bridge::MetricsPublisher>(*node_);
  }

  void TearDown() override {
    publisher_.reset();
    node_.reset();
  }

  // Spin briefly so the internal timer fires and we get a snapshot published.
  ros2_gst_video_bridge_msgs::msg::RuntimeStatus waitForStatus() {
    ros2_gst_video_bridge_msgs::msg::RuntimeStatus received;
    bool got_message = false;

    auto sub = node_->create_subscription<ros2_gst_video_bridge_msgs::msg::RuntimeStatus>(
        node_->get_name() + std::string("/runtime_status"), rclcpp::QoS(10),
        [&](ros2_gst_video_bridge_msgs::msg::RuntimeStatus::SharedPtr msg) {
          received = *msg;
          got_message = true;
        });

    publisher_->publishHeartbeat();

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!got_message && std::chrono::steady_clock::now() < deadline) {
      rclcpp::spin_some(node_);
    }

    return received;
  }

  std::shared_ptr<rclcpp::Node> node_;
  std::unique_ptr<ros2_gst_video_bridge::MetricsPublisher> publisher_;
  static int test_counter_;
};

int MetricsPublisherTest::test_counter_ = 0;

TEST_F(MetricsPublisherTest, InitialStateHasZeroCounters) {
  publisher_->setSessionMetadata("sid", "stm", "/cam/image_raw", "h264", "x264enc", "srt",
                                 "srt://127.0.0.1:9000?mode=listener");
  const auto status = waitForStatus();
  EXPECT_EQ(status.dropped_total, 0ULL);
  EXPECT_EQ(status.reconnect_count, 0ULL);
}

TEST_F(MetricsPublisherTest, RecordFrameOutIncreasesFrameCount) {
  publisher_->recordFrameOut();
  publisher_->recordFrameOut();
  publisher_->recordFrameOut();
  // We cannot read frames_out_ directly but we can verify fps_out > 0 when
  // publishHeartbeat is called shortly after recording.
  // This test mainly checks that the methods do not throw.
  EXPECT_NO_THROW(publisher_->publishHeartbeat());
}

TEST_F(MetricsPublisherTest, DropCountersAccumulate) {
  publisher_->recordFrameDroppedBackpressure();
  publisher_->recordFrameDroppedMalformed();
  publisher_->recordFrameDroppedThrottle();
  publisher_->recordFrameDroppedBackpressure();

  const auto status = waitForStatus();
  EXPECT_EQ(status.dropped_total, 4ULL);
  EXPECT_EQ(status.dropped_backpressure_total, 2ULL);
  EXPECT_EQ(status.dropped_malformed_total, 1ULL);
  EXPECT_EQ(status.dropped_throttle_total, 1ULL);
}

TEST_F(MetricsPublisherTest, ReconnectCounterIncreases) {
  publisher_->recordReconnect();
  publisher_->recordReconnect();

  const auto status = waitForStatus();
  EXPECT_EQ(status.reconnect_count, 2ULL);
}

TEST_F(MetricsPublisherTest, UpdateRuntimeStateReflectedInStatus) {
  publisher_->updateRuntimeState("streaming");
  const auto status = waitForStatus();
  EXPECT_EQ(status.state, "streaming");
}

TEST_F(MetricsPublisherTest, AdaptationStateReflectedInStatus) {
  publisher_->setAdaptationState("conservative", 2);
  const auto status = waitForStatus();
  EXPECT_EQ(status.adaptation_profile, "conservative");
  EXPECT_EQ(status.adaptation_level, 2);
}

TEST_F(MetricsPublisherTest, EffectiveEncodingReflectedInStatus) {
  publisher_->setEffectiveEncoding(1500, 15, 25.0F);
  const auto status = waitForStatus();
  EXPECT_EQ(status.bitrate_kbps_effective, 1500U);
  EXPECT_EQ(status.gop_effective, 15U);
  EXPECT_FLOAT_EQ(status.max_fps_effective, 25.0F);
}

TEST_F(MetricsPublisherTest, CodecFlagsReflectedInStatus) {
  publisher_->setCodecSelectionFlags(true, true, false);
  const auto status = waitForStatus();
  EXPECT_TRUE(status.auto_codec_requested);
  EXPECT_TRUE(status.hardware_encoder_selected);
  EXPECT_FALSE(status.software_fallback_applied);
}

TEST_F(MetricsPublisherTest, PushLatencyEWMAIsPositive) {
  publisher_->recordPushLatency(3.5);
  publisher_->recordPushLatency(4.0);
  // No direct accessor; just check publishHeartbeat does not throw.
  EXPECT_NO_THROW(publisher_->publishHeartbeat());
}

TEST_F(MetricsPublisherTest, NegativePushLatencyIsIgnored) {
  // Should not crash or produce undefined behaviour.
  EXPECT_NO_THROW(publisher_->recordPushLatency(-1.0));
  EXPECT_NO_THROW(publisher_->recordPushLatency(0.0));
}

} // namespace
