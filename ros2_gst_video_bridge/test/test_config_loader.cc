#include "ros2_gst_video_bridge/core/config_loader.hpp"

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>

#include <vector>

namespace {

class ConfigLoaderTest : public ::testing::Test {
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
};

TEST_F(ConfigLoaderTest, AppliesJetsonLowLatencyProfileDefaults) {
  rclcpp::NodeOptions options;
  options.parameter_overrides(
      std::vector<rclcpp::Parameter>{rclcpp::Parameter("profile.machine", "jetson"),
                                     rclcpp::Parameter("profile.stream", "low_latency")});

  auto node = std::make_shared<rclcpp::Node>("config_loader_profile_test", options);
  const auto cfg = ros2_gst_video_bridge::ConfigLoader::loadFromNode(*node);

  EXPECT_EQ(cfg.profile.machine, "jetson");
  EXPECT_EQ(cfg.profile.stream, "low_latency");
  EXPECT_EQ(cfg.transport.kind, "srt");
  EXPECT_EQ(cfg.transport.latency_ms, 60);
  EXPECT_EQ(cfg.codec.name, "auto");
  EXPECT_EQ(cfg.codec.tune, "zerolatency");
  EXPECT_GT(cfg.codec.bitrate_kbps, 0);
}

TEST_F(ConfigLoaderTest, ValidateRejectsInvalidMode) {
  ros2_gst_video_bridge::GstBridgeConfig cfg;
  cfg.runtime.mode = "invalid_mode";

  const auto errors = ros2_gst_video_bridge::ConfigLoader::validate(cfg);
  EXPECT_FALSE(errors.empty());
}

TEST_F(ConfigLoaderTest, ValidateAcceptsAutoAndAv1Codecs) {
  ros2_gst_video_bridge::GstBridgeConfig cfg;
  cfg.codec.name = "auto";

  auto errors = ros2_gst_video_bridge::ConfigLoader::validate(cfg);
  EXPECT_TRUE(errors.empty());

  cfg.codec.name = "av1";
  errors = ros2_gst_video_bridge::ConfigLoader::validate(cfg);
  EXPECT_TRUE(errors.empty());
}

TEST_F(ConfigLoaderTest, LoadsBackpressureRecoveryParameters) {
  rclcpp::NodeOptions options;
  options.parameter_overrides(std::vector<rclcpp::Parameter>{
      rclcpp::Parameter("runtime.backpressure.reconnect_after_ms", 1500),
      rclcpp::Parameter("runtime.backpressure.max_consecutive_drops", 45),
  });

  auto node = std::make_shared<rclcpp::Node>("config_loader_backpressure_test", options);
  const auto cfg = ros2_gst_video_bridge::ConfigLoader::loadFromNode(*node);

  EXPECT_EQ(cfg.runtime.backpressure_reconnect_after_ms, 1500);
  EXPECT_EQ(cfg.runtime.backpressure_max_consecutive_drops, 45);
}

TEST_F(ConfigLoaderTest, ValidateRejectsInvalidBackpressureRecoveryParameters) {
  ros2_gst_video_bridge::GstBridgeConfig cfg;
  cfg.runtime.backpressure_reconnect_after_ms = 0;
  cfg.runtime.backpressure_max_consecutive_drops = -1;

  const auto errors = ros2_gst_video_bridge::ConfigLoader::validate(cfg);
  EXPECT_GE(errors.size(), 2U);
}

TEST_F(ConfigLoaderTest, ModernParametersOverrideBackwardCompatibleAliases) {
  rclcpp::NodeOptions options;
  options.parameter_overrides(std::vector<rclcpp::Parameter>{
      rclcpp::Parameter("gst.transport", "srt"),
      rclcpp::Parameter("transport.kind", "file"),
      rclcpp::Parameter("gst.sink_uri", "srt://127.0.0.1:9000?mode=listener"),
      rclcpp::Parameter("transport.sink_uri", "/tmp/bridge.ts"),
      rclcpp::Parameter("gst.bitrate_kbps", 2000),
      rclcpp::Parameter("codec.bitrate_kbps", 1500),
  });

  auto node = std::make_shared<rclcpp::Node>("config_loader_alias_precedence_test", options);
  const auto cfg = ros2_gst_video_bridge::ConfigLoader::loadFromNode(*node);

  EXPECT_EQ(cfg.transport.kind, "file");
  EXPECT_EQ(cfg.transport.sink_uri, "/tmp/bridge.ts");
  EXPECT_EQ(cfg.codec.bitrate_kbps, 1500);
}

} // namespace
