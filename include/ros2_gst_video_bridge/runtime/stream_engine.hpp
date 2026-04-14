// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__RUNTIME__STREAM_ENGINE_HPP_
#define ROS2_GST_VIDEO_BRIDGE__RUNTIME__STREAM_ENGINE_HPP_

#include <gst/gst.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>

namespace ros2_gst_video_bridge {

class StreamEngine {
public:
  explicit StreamEngine(std::string pipeline);
  ~StreamEngine();

  bool start();
  void stop();
  bool isRunning() const;
  const std::string& pipeline() const;
  bool pushFrame(const uint8_t* data, size_t size, int width, int height,
                 const std::string& gst_format, uint64_t timestamp_ns);
  std::string lastError() const;

private:
  bool setAppSrcCaps(int width, int height, const std::string& gst_format);
  void processBusMessages(GstClockTime timeout);
  void busLoop(std::stop_token stop_token);

  std::string pipeline_;
  std::string last_error_;
  GstElement* pipeline_element_{nullptr};
  GstElement* appsrc_{nullptr};
  GstBus* bus_{nullptr};

  int caps_width_{0};
  int caps_height_{0};
  std::string caps_format_;

  std::atomic_bool running_{false};
  bool gst_initialized_{false};
  std::jthread bus_thread_;
  mutable std::mutex mutex_;
};

} // namespace ros2_gst_video_bridge

#endif // ROS2_GST_VIDEO_BRIDGE__RUNTIME__STREAM_ENGINE_HPP_
