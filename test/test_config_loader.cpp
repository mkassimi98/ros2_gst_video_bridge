#include "ros2_gst_video_bridge/core/config_loader.hpp"

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>

#include <vector>

namespace
{

class ConfigLoaderTest : public ::testing::Test
{
protected:
  static void SetUpTestSuite()
  {
    if (!rclcpp::ok()) {
      rclcpp::init(0, nullptr);
    }
  }

  static void TearDownTestSuite()
  {
    if (rclcpp::ok()) {
      rclcpp::shutdown();
    }
  }
};

TEST_F(ConfigLoaderTest, AppliesJetsonLowLatencyProfileDefaults)
{
  rclcpp::NodeOptions options;
  options.parameter_overrides(std::vector<rclcpp::Parameter>{
    rclcpp::Parameter("profile.machine", "jetson"),
    rclcpp::Parameter("profile.stream", "low_latency")
  });

  auto node = std::make_shared<rclcpp::Node>("config_loader_profile_test", options);
  const auto cfg = ros2_gst_video_bridge::ConfigLoader::loadFromNode(*node);

  EXPECT_EQ(cfg.profile.machine, "jetson");
  EXPECT_EQ(cfg.profile.stream, "low_latency");
  EXPECT_EQ(cfg.transport.kind, "srt");
  EXPECT_EQ(cfg.transport.latency_ms, 60);
  EXPECT_EQ(cfg.codec.name, "h264");
  EXPECT_EQ(cfg.codec.tune, "zerolatency");
  EXPECT_GT(cfg.codec.bitrate_kbps, 0);
}

TEST_F(ConfigLoaderTest, ValidateRejectsInvalidMode)
{
  ros2_gst_video_bridge::GstBridgeConfig cfg;
  cfg.runtime.mode = "invalid_mode";

  const auto errors = ros2_gst_video_bridge::ConfigLoader::validate(cfg);
  EXPECT_FALSE(errors.empty());
}

}  // namespace
