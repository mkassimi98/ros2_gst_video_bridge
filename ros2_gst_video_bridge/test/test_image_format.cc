#include "ros2_gst_video_bridge/runtime/image_format.hpp"

#include <gtest/gtest.h>
#include <sensor_msgs/image_encodings.hpp>

namespace {

sensor_msgs::msg::Image makeImage() {
  sensor_msgs::msg::Image image;
  image.width = 4;
  image.height = 3;
  image.encoding = sensor_msgs::image_encodings::BGR8;
  image.step = 12;
  image.data.assign(36, 0);
  return image;
}

TEST(ImageFormatTest, ValidatesWellFormedImage) {
  const auto result = ros2_gst_video_bridge::validateImageShape(makeImage());

  EXPECT_TRUE(result.valid);
  EXPECT_TRUE(result.error.empty());
}

TEST(ImageFormatTest, RejectsUnsupportedEncoding) {
  auto image = makeImage();
  image.encoding = "unsupported";

  const auto result = ros2_gst_video_bridge::validateImageShape(image);

  EXPECT_FALSE(result.valid);
  EXPECT_NE(result.error.find("unsupported image encoding"), std::string::npos);
}

TEST(ImageFormatTest, RejectsInvalidStep) {
  auto image = makeImage();
  image.step = 2;

  const auto result = ros2_gst_video_bridge::validateImageShape(image);

  EXPECT_FALSE(result.valid);
  EXPECT_NE(result.error.find("smaller than expected row size"), std::string::npos);
}

TEST(ImageFormatTest, RejectsTruncatedData) {
  auto image = makeImage();
  image.data.resize(4);

  const auto result = ros2_gst_video_bridge::validateImageShape(image);

  EXPECT_FALSE(result.valid);
  EXPECT_NE(result.error.find("smaller than step*height"), std::string::npos);
}

TEST(ImageFormatTest, ResolvesBayerAsGrayFallback) {
  std::string gst_format;

  EXPECT_TRUE(ros2_gst_video_bridge::resolveGstFormat(sensor_msgs::image_encodings::BAYER_RGGB8,
                                                      gst_format));
  EXPECT_EQ(gst_format, "GRAY8");
  EXPECT_TRUE(ros2_gst_video_bridge::isBayerEncoding(sensor_msgs::image_encodings::BAYER_RGGB8));
}

TEST(ImageFormatTest, ResolvesGstBayerPatternFor8BitEncodings) {
  EXPECT_EQ(ros2_gst_video_bridge::resolveGstBayerFormat(
                sensor_msgs::image_encodings::BAYER_RGGB8),
            "rggb");
  EXPECT_EQ(ros2_gst_video_bridge::resolveGstBayerFormat(
                sensor_msgs::image_encodings::BAYER_BGGR8),
            "bggr");
}

TEST(ImageFormatTest, DoesNotResolveGstBayerPatternFor16BitEncodings) {
  EXPECT_TRUE(ros2_gst_video_bridge::resolveGstBayerFormat(
                  sensor_msgs::image_encodings::BAYER_RGGB16)
                  .empty());
}

} // namespace
