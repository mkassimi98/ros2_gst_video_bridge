// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/runtime/image_format.hpp"

#include <sensor_msgs/image_encodings.hpp>

#include <sstream>

namespace ros2_gst_video_bridge {

std::string resolveGstBayerFormat(const std::string& encoding) {
  if (encoding == sensor_msgs::image_encodings::BAYER_RGGB8) {
    return "rggb";
  }
  if (encoding == sensor_msgs::image_encodings::BAYER_BGGR8) {
    return "bggr";
  }
  if (encoding == sensor_msgs::image_encodings::BAYER_GBRG8) {
    return "gbrg";
  }
  if (encoding == sensor_msgs::image_encodings::BAYER_GRBG8) {
    return "grbg";
  }
  return "";
}

bool resolveGstFormat(const std::string& encoding, std::string& gst_format) {
  if (encoding == sensor_msgs::image_encodings::BGR8 ||
      encoding == sensor_msgs::image_encodings::TYPE_8UC3 ||
      encoding == sensor_msgs::image_encodings::TYPE_8SC3) {
    gst_format = "BGR";
    return true;
  }

  if (encoding == sensor_msgs::image_encodings::RGB8) {
    gst_format = "RGB";
    return true;
  }

  if (encoding == sensor_msgs::image_encodings::BGRA8 ||
      encoding == sensor_msgs::image_encodings::TYPE_8UC4 ||
      encoding == sensor_msgs::image_encodings::TYPE_8SC4) {
    gst_format = "BGRA";
    return true;
  }

  if (encoding == sensor_msgs::image_encodings::RGBA8) {
    gst_format = "RGBA";
    return true;
  }

  if (encoding == sensor_msgs::image_encodings::MONO8 ||
      encoding == sensor_msgs::image_encodings::TYPE_8UC1 ||
      encoding == sensor_msgs::image_encodings::TYPE_8SC1) {
    gst_format = "GRAY8";
    return true;
  }

  if (encoding == sensor_msgs::image_encodings::MONO16 ||
      encoding == sensor_msgs::image_encodings::TYPE_16UC1 ||
      encoding == sensor_msgs::image_encodings::TYPE_16SC1) {
    gst_format = "GRAY16_LE";
    return true;
  }

  if (!resolveGstBayerFormat(encoding).empty()) {
    gst_format = "GRAY8";
    return true;
  }

  if (encoding == sensor_msgs::image_encodings::BAYER_RGGB16 ||
      encoding == sensor_msgs::image_encodings::BAYER_BGGR16 ||
      encoding == sensor_msgs::image_encodings::BAYER_GBRG16 ||
      encoding == sensor_msgs::image_encodings::BAYER_GRBG16) {
    gst_format = "GRAY16_LE";
    return true;
  }

  return false;
}

bool isBayerEncoding(const std::string& encoding) {
  return encoding == sensor_msgs::image_encodings::BAYER_RGGB8 ||
         encoding == sensor_msgs::image_encodings::BAYER_BGGR8 ||
         encoding == sensor_msgs::image_encodings::BAYER_GBRG8 ||
         encoding == sensor_msgs::image_encodings::BAYER_GRBG8 ||
         encoding == sensor_msgs::image_encodings::BAYER_RGGB16 ||
         encoding == sensor_msgs::image_encodings::BAYER_BGGR16 ||
         encoding == sensor_msgs::image_encodings::BAYER_GBRG16 ||
         encoding == sensor_msgs::image_encodings::BAYER_GRBG16;
}

size_t bytesPerPixelForEncoding(const std::string& encoding) {
  if (encoding == sensor_msgs::image_encodings::BGR8 ||
      encoding == sensor_msgs::image_encodings::RGB8 ||
      encoding == sensor_msgs::image_encodings::TYPE_8UC3 ||
      encoding == sensor_msgs::image_encodings::TYPE_8SC3) {
    return 3;
  }

  if (encoding == sensor_msgs::image_encodings::BGRA8 ||
      encoding == sensor_msgs::image_encodings::RGBA8 ||
      encoding == sensor_msgs::image_encodings::TYPE_8UC4 ||
      encoding == sensor_msgs::image_encodings::TYPE_8SC4) {
    return 4;
  }

  if (encoding == sensor_msgs::image_encodings::MONO8 ||
      encoding == sensor_msgs::image_encodings::TYPE_8UC1 ||
      encoding == sensor_msgs::image_encodings::TYPE_8SC1 ||
      encoding == sensor_msgs::image_encodings::BAYER_RGGB8 ||
      encoding == sensor_msgs::image_encodings::BAYER_BGGR8 ||
      encoding == sensor_msgs::image_encodings::BAYER_GBRG8 ||
      encoding == sensor_msgs::image_encodings::BAYER_GRBG8) {
    return 1;
  }

  if (encoding == sensor_msgs::image_encodings::MONO16 ||
      encoding == sensor_msgs::image_encodings::TYPE_16UC1 ||
      encoding == sensor_msgs::image_encodings::TYPE_16SC1 ||
      encoding == sensor_msgs::image_encodings::BAYER_RGGB16 ||
      encoding == sensor_msgs::image_encodings::BAYER_BGGR16 ||
      encoding == sensor_msgs::image_encodings::BAYER_GBRG16 ||
      encoding == sensor_msgs::image_encodings::BAYER_GRBG16) {
    return 2;
  }

  return 0;
}

ImageShapeValidation validateImageShape(const sensor_msgs::msg::Image& msg) {
  if (msg.width == 0 || msg.height == 0) {
    return {false, "image width and height must be non-zero"};
  }

  const size_t bytes_per_pixel = bytesPerPixelForEncoding(msg.encoding);
  if (bytes_per_pixel == 0) {
    return {false, "unsupported image encoding '" + msg.encoding + "'"};
  }

  const size_t min_step = static_cast<size_t>(msg.width) * bytes_per_pixel;
  if (msg.step < min_step) {
    std::ostringstream ss;
    ss << "image step " << msg.step << " is smaller than expected row size " << min_step;
    return {false, ss.str()};
  }

  const size_t min_size = static_cast<size_t>(msg.step) * static_cast<size_t>(msg.height);
  if (msg.data.size() < min_size) {
    std::ostringstream ss;
    ss << "image data size " << msg.data.size() << " is smaller than step*height " << min_size;
    return {false, ss.str()};
  }

  return {true, ""};
}

} // namespace ros2_gst_video_bridge
