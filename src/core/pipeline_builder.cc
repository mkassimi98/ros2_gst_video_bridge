// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/core/pipeline_builder.hpp"

#include <sstream>

namespace ros2_gst_video_bridge
{

std::string PipelineBuilder::build(const GstBridgeConfig & config)
{
  if (!config.gst_pipeline_override.empty()) {
    return config.gst_pipeline_override;
  }

  std::ostringstream pipeline;
  pipeline << "appsrc is-live=true format=time do-timestamp=true ! "
           << "videoconvert ! ";

  if (config.gst_codec == "h265") {
    pipeline << "x265enc tune=zerolatency bitrate=" << config.gst_bitrate_kbps << " ! ";
  } else {
    pipeline << "x264enc tune=zerolatency bitrate=" << config.gst_bitrate_kbps
             << " speed-preset=ultrafast ! ";
  }

  pipeline << "mpegtsmux ! ";

  if (config.gst_transport == "udp") {
    pipeline << "udpsink host=127.0.0.1 port=5000 sync=false async=false";
  } else if (config.gst_transport == "rtsp") {
    pipeline << "rtspclientsink location=" << config.gst_sink_uri;
  } else {
    pipeline << "srtsink uri=" << config.gst_sink_uri;
  }

  return pipeline.str();
}

}  // namespace ros2_gst_video_bridge
