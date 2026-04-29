// Tests for the internal detail/ utility headers.
// These are pure header-only functions with no dependencies beyond the STL.

#include "ros2_gst_video_bridge/detail/adaptation.hpp"
#include "ros2_gst_video_bridge/detail/encoder_selection.hpp"
#include "ros2_gst_video_bridge/detail/string_utils.hpp"

#include <gtest/gtest.h>

namespace {

// ─── string_utils ────────────────────────────────────────────────────────────

TEST(StringUtilsTest, StartsWithReturnsTrueOnMatch) {
  EXPECT_TRUE(ros2_gst_video_bridge::detail::startsWith("h264 -> x264enc [sw]", "h264 -> "));
}

TEST(StringUtilsTest, StartsWithReturnsFalseOnMismatch) {
  EXPECT_FALSE(ros2_gst_video_bridge::detail::startsWith("h265 -> x265enc [sw]", "h264 -> "));
}

TEST(StringUtilsTest, StartsWithEmptyPrefixAlwaysTrue) {
  EXPECT_TRUE(ros2_gst_video_bridge::detail::startsWith("anything", ""));
}

TEST(StringUtilsTest, ExtractElementNameParsesCorrectly) {
  EXPECT_EQ(ros2_gst_video_bridge::detail::extractElementName("h264 -> x264enc [sw]"), "x264enc");
}

TEST(StringUtilsTest, ExtractElementNameParsesHwTag) {
  EXPECT_EQ(
      ros2_gst_video_bridge::detail::extractElementName("h264 -> nvv4l2h264enc [hw:nvidia-v4l2]"),
      "nvv4l2h264enc");
}

TEST(StringUtilsTest, ExtractElementNameReturnsEmptyOnMalformed) {
  EXPECT_TRUE(ros2_gst_video_bridge::detail::extractElementName("no_arrow_here").empty());
  EXPECT_TRUE(ros2_gst_video_bridge::detail::extractElementName("h264 -> ").empty());
}

// ─── encoder_selection ───────────────────────────────────────────────────────

TEST(EncoderSelectionTest, FindsSwEncoderForH264) {
  const std::vector<std::string> impls = {
      "h264 -> nvv4l2h264enc [hw:nvidia-v4l2]",
      "h264 -> x264enc [sw]",
      "h265 -> x265enc [sw]",
  };
  EXPECT_EQ(ros2_gst_video_bridge::detail::selectSoftwareEncoderForCodec("h264", impls), "x264enc");
}

TEST(EncoderSelectionTest, FindsSwEncoderForAv1) {
  const std::vector<std::string> impls = {
      "av1 -> nvv4l2av1enc [hw:nvidia-v4l2]",
      "av1 -> av1enc [sw]",
      "h265 -> x265enc [sw]",
  };
  EXPECT_EQ(ros2_gst_video_bridge::detail::selectSoftwareEncoderForCodec("av1", impls), "av1enc");
}

TEST(EncoderSelectionTest, IgnoresHardwareEncoders) {
  const std::vector<std::string> impls = {
      "h264 -> nvv4l2h264enc [hw:nvidia-v4l2]",
  };
  EXPECT_TRUE(ros2_gst_video_bridge::detail::selectSoftwareEncoderForCodec("h264", impls).empty());
}

TEST(EncoderSelectionTest, DoesNotMatchOtherCodecs) {
  const std::vector<std::string> impls = {
      "h265 -> x265enc [sw]",
  };
  EXPECT_TRUE(ros2_gst_video_bridge::detail::selectSoftwareEncoderForCodec("h264", impls).empty());
}

TEST(EncoderSelectionTest, EmptyListReturnsEmpty) {
  EXPECT_TRUE(ros2_gst_video_bridge::detail::selectSoftwareEncoderForCodec("h264", {}).empty());
}

TEST(EncoderSelectionTest, FindsFirstSwMatchWhenMultiplePresent) {
  const std::vector<std::string> impls = {
      "h264 -> openh264enc [sw]",
      "h264 -> x264enc [sw]",
  };
  // Must return the first matching SW encoder in list order.
  EXPECT_EQ(ros2_gst_video_bridge::detail::selectSoftwareEncoderForCodec("h264", impls),
            "openh264enc");
}

// ─── adaptation ──────────────────────────────────────────────────────────────

TEST(AdaptationTest, LevelZeroIsAlwaysNoChange) {
  for (const auto* profile : {"conservative", "balanced", "aggressive", "unknown"}) {
    const auto scales = ros2_gst_video_bridge::detail::computeAdaptationScales(profile, 0);
    EXPECT_DOUBLE_EQ(scales.fps_scale, 1.0) << "profile=" << profile;
    EXPECT_DOUBLE_EQ(scales.bitrate_scale, 1.0) << "profile=" << profile;
  }
}

TEST(AdaptationTest, ConservativeLevel1) {
  const auto s = ros2_gst_video_bridge::detail::computeAdaptationScales("conservative", 1);
  EXPECT_DOUBLE_EQ(s.fps_scale, 0.85);
  EXPECT_DOUBLE_EQ(s.bitrate_scale, 0.75);
}

TEST(AdaptationTest, ConservativeLevel2) {
  const auto s = ros2_gst_video_bridge::detail::computeAdaptationScales("conservative", 2);
  EXPECT_DOUBLE_EQ(s.fps_scale, 0.70);
  EXPECT_DOUBLE_EQ(s.bitrate_scale, 0.55);
}

TEST(AdaptationTest, AggressiveLevel1HasMoreReductionThanConservative) {
  const auto cons = ros2_gst_video_bridge::detail::computeAdaptationScales("conservative", 1);
  const auto agg = ros2_gst_video_bridge::detail::computeAdaptationScales("aggressive", 1);
  EXPECT_LT(agg.fps_scale, cons.fps_scale);
  EXPECT_LT(agg.bitrate_scale, cons.bitrate_scale);
}

TEST(AdaptationTest, AggressiveLevel2HasMoreReductionThanConservative) {
  const auto cons = ros2_gst_video_bridge::detail::computeAdaptationScales("conservative", 2);
  const auto agg = ros2_gst_video_bridge::detail::computeAdaptationScales("aggressive", 2);
  EXPECT_LT(agg.fps_scale, cons.fps_scale);
  EXPECT_LT(agg.bitrate_scale, cons.bitrate_scale);
}

TEST(AdaptationTest, BalancedIsBetweenConservativeAndAggressive) {
  const auto cons = ros2_gst_video_bridge::detail::computeAdaptationScales("conservative", 1);
  const auto bal = ros2_gst_video_bridge::detail::computeAdaptationScales("balanced", 1);
  const auto agg = ros2_gst_video_bridge::detail::computeAdaptationScales("aggressive", 1);
  EXPECT_LT(agg.fps_scale, bal.fps_scale);
  EXPECT_LT(bal.fps_scale, cons.fps_scale);
}

TEST(AdaptationTest, LevelAbove2IsClamped) {
  const auto s2 = ros2_gst_video_bridge::detail::computeAdaptationScales("balanced", 2);
  const auto s3 = ros2_gst_video_bridge::detail::computeAdaptationScales("balanced", 3);
  EXPECT_DOUBLE_EQ(s2.fps_scale, s3.fps_scale);
  EXPECT_DOUBLE_EQ(s2.bitrate_scale, s3.bitrate_scale);
}

TEST(AdaptationTest, UnknownProfileFallsBackToBalanced) {
  const auto bal = ros2_gst_video_bridge::detail::computeAdaptationScales("balanced", 1);
  const auto unk = ros2_gst_video_bridge::detail::computeAdaptationScales("unknown_profile", 1);
  EXPECT_DOUBLE_EQ(bal.fps_scale, unk.fps_scale);
  EXPECT_DOUBLE_EQ(bal.bitrate_scale, unk.bitrate_scale);
}

} // namespace
