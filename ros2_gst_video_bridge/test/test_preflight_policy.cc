#include "ros2_gst_video_bridge/detail/preflight_policy.hpp"

#include <gtest/gtest.h>

namespace {

TEST(PreflightPolicyTest, KeepsRequestedTransportWhenAvailable) {
  const std::vector<std::string> sinks{"srt", "udp", "file"};
  const auto result =
      ros2_gst_video_bridge::detail::resolveTransportForAvailableSinks("srt", sinks);

  EXPECT_EQ(result.effective_transport, "srt");
  EXPECT_FALSE(result.fallback_applied);
}

TEST(PreflightPolicyTest, FallsBackTransportByPriorityWhenUnavailable) {
  const std::vector<std::string> sinks{"udp", "file"};
  const auto result =
      ros2_gst_video_bridge::detail::resolveTransportForAvailableSinks("rtsp", sinks);

  EXPECT_EQ(result.effective_transport, "udp");
  EXPECT_TRUE(result.fallback_applied);
}

TEST(PreflightPolicyTest, KeepsEncoderWhenImplementationExists) {
  const std::vector<std::string> implementations{
      "h264 -> x264enc [sw]",
      "h264 -> nvv4l2h264enc [hw:nvidia-v4l2]",
  };

  const auto result = ros2_gst_video_bridge::detail::resolveEncoderOverride("h264", "nvv4l2h264enc",
                                                                            implementations);

  EXPECT_EQ(result.effective_encoder, "nvv4l2h264enc");
  EXPECT_FALSE(result.fallback_applied);
}

TEST(PreflightPolicyTest, FallsBackToSoftwareEncoderWhenOverrideUnavailable) {
  const std::vector<std::string> implementations{
      "av1 -> av1enc [sw]",
      "h264 -> x264enc [sw]",
      "h265 -> x265enc [sw]",
  };

  const auto result =
      ros2_gst_video_bridge::detail::resolveEncoderOverride("h264", "missingenc", implementations);

  EXPECT_EQ(result.effective_encoder, "x264enc");
  EXPECT_TRUE(result.fallback_applied);
}

TEST(PreflightPolicyTest, FallsBackToSoftwareAv1EncoderWhenOverrideUnavailable) {
  const std::vector<std::string> implementations{
      "av1 -> av1enc [sw]",
      "av1 -> nvv4l2av1enc [hw:nvidia-v4l2]",
  };

  const auto result =
      ros2_gst_video_bridge::detail::resolveEncoderOverride("av1", "missingenc", implementations);

  EXPECT_EQ(result.effective_encoder, "av1enc");
  EXPECT_TRUE(result.fallback_applied);
}

TEST(PreflightPolicyTest, ClearsEncoderWhenNoFallbackExists) {
  const std::vector<std::string> implementations{"h265 -> x265enc [sw]"};

  const auto result =
      ros2_gst_video_bridge::detail::resolveEncoderOverride("h264", "missingenc", implementations);

  EXPECT_TRUE(result.effective_encoder.empty());
  EXPECT_TRUE(result.fallback_applied);
}

} // namespace
