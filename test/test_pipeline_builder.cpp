#include "ros2_gst_video_bridge/core/pipeline_builder.hpp"

#include <gtest/gtest.h>

namespace
{

TEST(PipelineBuilderTest, BuildsSrtH264Pipeline)
{
  ros2_gst_video_bridge::GstBridgeConfig cfg;
  cfg.transport.kind = "srt";
  cfg.transport.sink_uri = "srt://127.0.0.1:9000?mode=listener";
  cfg.codec.name = "h264";
  cfg.codec.bitrate_kbps = 2000;
  cfg.codec.gop = 30;

  const auto pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);

  EXPECT_NE(pipeline.find("appsrc name=bridge_appsrc"), std::string::npos);
  EXPECT_NE(pipeline.find("x264enc"), std::string::npos);
  EXPECT_NE(pipeline.find("srtsink"), std::string::npos);
}

TEST(PipelineBuilderTest, BuildsUdpPipelineWithParsedHostAndPort)
{
  ros2_gst_video_bridge::GstBridgeConfig cfg;
  cfg.transport.kind = "udp";
  cfg.transport.sink_uri = "udp://192.168.1.10:6000";
  cfg.codec.name = "h264";

  const auto pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);

  EXPECT_NE(pipeline.find("rtph264pay"), std::string::npos);
  EXPECT_NE(pipeline.find("udpsink host=192.168.1.10 port=6000"), std::string::npos);
}

}  // namespace
