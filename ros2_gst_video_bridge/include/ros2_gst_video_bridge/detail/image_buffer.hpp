#ifndef ROS2_GST_VIDEO_BRIDGE__DETAIL__IMAGE_BUFFER_HPP_
#define ROS2_GST_VIDEO_BRIDGE__DETAIL__IMAGE_BUFFER_HPP_

#include <gst/gst.h>
#include <gst/video/video.h>

#include <limits>
#include <sensor_msgs/msg/image.hpp>

namespace ros2_gst_video_bridge::detail {

inline GstBuffer *wrapImageBuffer(sensor_msgs::msg::Image::UniquePtr image,
                                  const std::string &gst_format) {
  if (!image || image->data.empty() || image->width == 0 || image->height == 0 ||
      image->step == 0 || image->step > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
      image->data.size() / image->step < image->height) {
    return nullptr;
  }

  auto *owner = image.get();
  GstBuffer *buffer = gst_buffer_new_wrapped_full(
      GST_MEMORY_FLAG_READONLY, owner->data.data(), owner->data.size(), 0, owner->data.size(),
      owner, [](gpointer data) { delete static_cast<sensor_msgs::msg::Image *>(data); });
  if (!buffer) {
    return nullptr;
  }
  image.release();

  // Preserve ROS row stride, including padding. Bayer uses video/x-bayer caps
  // rather than GstVideoFormat and must use tightly packed rows.
  const auto format = gst_video_format_from_string(gst_format.c_str());
  if (format != GST_VIDEO_FORMAT_UNKNOWN) {
    gsize offsets[GST_VIDEO_MAX_PLANES]{};
    gint strides[GST_VIDEO_MAX_PLANES]{};
    strides[0] = static_cast<gint>(owner->step);
    if (!gst_buffer_add_video_meta_full(buffer, GST_VIDEO_FRAME_FLAG_NONE, format, owner->width,
                                        owner->height, 1, offsets, strides)) {
      gst_buffer_unref(buffer);
      return nullptr;
    }
  } else if (owner->step != owner->width) {
    gst_buffer_unref(buffer);
    return nullptr;
  }
  return buffer;
}

} // namespace ros2_gst_video_bridge::detail

#endif // ROS2_GST_VIDEO_BRIDGE__DETAIL__IMAGE_BUFFER_HPP_
