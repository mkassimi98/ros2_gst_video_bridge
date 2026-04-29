#include "ros2_gst_video_bridge/core/pipeline_builder.hpp"

#include <gtest/gtest.h>

namespace {

TEST(PipelineBuilderTest, BuildsSrtH264Pipeline) {
  ros2_gst_video_bridge::GstBridgeConfig cfg;
  cfg.transport.kind = "srt";
  cfg.transport.sink_uri = "srt://127.0.0.1:9000?mode=listener";
  cfg.codec.name = "h264";
  cfg.codec.bitrate_kbps = 2000;
  cfg.codec.gop = 30;

  const auto pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);

  EXPECT_NE(pipeline.find("appsrc name=bridge_appsrc"), std::string::npos);
  EXPECT_NE(pipeline.find("block=false"), std::string::npos);
  EXPECT_NE(pipeline.find("max-buffers=2"), std::string::npos);
  EXPECT_NE(pipeline.find("leaky-type=upstream"), std::string::npos);
  EXPECT_NE(pipeline.find("queue leaky=downstream"), std::string::npos);
  EXPECT_NE(pipeline.find("max-size-buffers=8"), std::string::npos);
  EXPECT_NE(pipeline.find("x264enc"), std::string::npos);
  EXPECT_NE(pipeline.find("h264parse"), std::string::npos);
  EXPECT_NE(pipeline.find("srtsink"), std::string::npos);
}

TEST(PipelineBuilderTest, BuildsUdpPipelineWithParsedHostAndPort) {
  ros2_gst_video_bridge::GstBridgeConfig cfg;
  cfg.transport.kind = "udp";
  cfg.transport.sink_uri = "udp://192.168.1.10:6000";
  cfg.codec.name = "h264";

  const auto pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);

  EXPECT_NE(pipeline.find("rtph264pay"), std::string::npos);
  EXPECT_NE(pipeline.find("h264parse"), std::string::npos);
  EXPECT_NE(pipeline.find("max-size-buffers=4"), std::string::npos);
  EXPECT_NE(pipeline.find("udpsink host=192.168.1.10 port=6000"), std::string::npos);
}

TEST(PipelineBuilderTest, BuildsHardwareEncoderPipelineWithoutX26xOnlyProperties) {
  ros2_gst_video_bridge::GstBridgeConfig cfg;
  cfg.transport.kind = "srt";
  cfg.transport.sink_uri = "srt://127.0.0.1:9000?mode=listener";
  cfg.codec.name = "h264";
  cfg.codec.encoder = "nvv4l2h264enc";
  cfg.codec.tune = "zerolatency";
  cfg.codec.bitrate_kbps = 2000;
  cfg.codec.gop = 30;

  const auto pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);

  EXPECT_NE(pipeline.find("nvv4l2h264enc"), std::string::npos);
  EXPECT_NE(pipeline.find("nvvidconv"), std::string::npos);
  EXPECT_NE(pipeline.find("video/x-raw(memory:NVMM),format=NV12"), std::string::npos);
  EXPECT_NE(pipeline.find("h264parse"), std::string::npos);
  EXPECT_EQ(pipeline.find(" tune="), std::string::npos);
  EXPECT_EQ(pipeline.find(" key-int-max="), std::string::npos);
  EXPECT_EQ(pipeline.find(" speed-preset="), std::string::npos);
}

TEST(PipelineBuilderTest, BuildsH265PipelinesAcrossTransports) {
  ros2_gst_video_bridge::GstBridgeConfig cfg;
  cfg.codec.name = "h265";

  cfg.transport.kind = "srt";
  cfg.transport.sink_uri = "srt://127.0.0.1:9000?mode=listener";
  auto pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("x265enc"), std::string::npos);
  EXPECT_NE(pipeline.find("h265parse"), std::string::npos);
  EXPECT_NE(pipeline.find("mpegtsmux"), std::string::npos);
  EXPECT_NE(pipeline.find("srtsink"), std::string::npos);
  EXPECT_NE(pipeline.find("poll-timeout=100"), std::string::npos);

  cfg.transport.kind = "udp";
  cfg.transport.sink_uri = "udp://10.1.2.3:7000";
  pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("rtph265pay"), std::string::npos);
  EXPECT_NE(pipeline.find("max-size-buffers=4"), std::string::npos);
  EXPECT_NE(pipeline.find("udpsink host=10.1.2.3 port=7000"), std::string::npos);

  cfg.transport.kind = "rtsp";
  cfg.transport.sink_uri = "rtsp://127.0.0.1:8554/live";
  pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("rtph265pay"), std::string::npos);
  EXPECT_NE(pipeline.find("max-size-buffers=16"), std::string::npos);
  EXPECT_NE(pipeline.find("rtspclientsink location=rtsp://127.0.0.1:8554/live protocols=tcp"),
            std::string::npos);

  cfg.transport.kind = "file";
  cfg.transport.sink_uri = "/tmp/out_h265.ts";
  pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("mpegtsmux"), std::string::npos);
  EXPECT_NE(pipeline.find("queue leaky=no max-size-buffers=32"), std::string::npos);
  EXPECT_NE(pipeline.find("filesink location=/tmp/out_h265.ts"), std::string::npos);
}

TEST(PipelineBuilderTest, BuildsAv1PipelinesAcrossTransports) {
  ros2_gst_video_bridge::GstBridgeConfig cfg;
  cfg.codec.name = "av1";
  cfg.codec.bitrate_kbps = 2200;

  cfg.transport.kind = "srt";
  cfg.transport.sink_uri = "srt://127.0.0.1:9000?mode=listener";
  auto pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("av1enc"), std::string::npos);
  EXPECT_NE(pipeline.find("target-bitrate=2200"), std::string::npos);
  EXPECT_NE(pipeline.find("av1parse"), std::string::npos);
  EXPECT_NE(pipeline.find("matroskamux"), std::string::npos);
  EXPECT_NE(pipeline.find("srtsink"), std::string::npos);

  cfg.transport.kind = "udp";
  cfg.transport.sink_uri = "udp://10.1.2.3:7200";
  pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("rtpav1pay"), std::string::npos);
  EXPECT_NE(pipeline.find("max-size-buffers=4"), std::string::npos);
  EXPECT_NE(pipeline.find("udpsink host=10.1.2.3 port=7200"), std::string::npos);

  cfg.transport.kind = "rtsp";
  cfg.transport.sink_uri = "rtsp://127.0.0.1:8554/av1";
  pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("rtpav1pay"), std::string::npos);
  EXPECT_NE(pipeline.find("max-size-buffers=16"), std::string::npos);
  EXPECT_NE(pipeline.find("rtspclientsink location=rtsp://127.0.0.1:8554/av1 protocols=tcp"),
            std::string::npos);

  cfg.transport.kind = "file";
  cfg.transport.sink_uri = "/tmp/out_av1.mkv";
  pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("matroskamux"), std::string::npos);
  EXPECT_NE(pipeline.find("queue leaky=no max-size-buffers=32"), std::string::npos);
  EXPECT_NE(pipeline.find("filesink location=/tmp/out_av1.mkv"), std::string::npos);
}

TEST(PipelineBuilderTest, BuildsMjpegPipelinesAcrossTransports) {
  ros2_gst_video_bridge::GstBridgeConfig cfg;
  cfg.codec.name = "mjpeg";

  cfg.transport.kind = "srt";
  cfg.transport.sink_uri = "srt://127.0.0.1:9000?mode=listener";
  auto pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("jpegenc"), std::string::npos);
  EXPECT_NE(pipeline.find("matroskamux"), std::string::npos);
  EXPECT_NE(pipeline.find("srtsink"), std::string::npos);

  cfg.transport.kind = "udp";
  cfg.transport.sink_uri = "udp://10.1.2.3:7100";
  pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("rtpjpegpay"), std::string::npos);
  EXPECT_NE(pipeline.find("max-size-buffers=4"), std::string::npos);
  EXPECT_NE(pipeline.find("udpsink host=10.1.2.3 port=7100"), std::string::npos);

  cfg.transport.kind = "rtsp";
  cfg.transport.sink_uri = "rtsp://127.0.0.1:8554/mjpeg";
  pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("rtpjpegpay"), std::string::npos);
  EXPECT_NE(pipeline.find("max-size-buffers=16"), std::string::npos);
  EXPECT_NE(pipeline.find("rtspclientsink location=rtsp://127.0.0.1:8554/mjpeg protocols=tcp"),
            std::string::npos);

  cfg.transport.kind = "file";
  cfg.transport.sink_uri = "/tmp/out_mjpeg.mkv";
  pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("matroskamux"), std::string::npos);
  EXPECT_NE(pipeline.find("queue leaky=no max-size-buffers=32"), std::string::npos);
  EXPECT_NE(pipeline.find("filesink location=/tmp/out_mjpeg.mkv"), std::string::npos);
}

TEST(PipelineBuilderTest, BuildsOpenH264AndVaapiChains) {
  ros2_gst_video_bridge::GstBridgeConfig cfg;
  cfg.transport.kind = "srt";
  cfg.transport.sink_uri = "srt://127.0.0.1:9000?mode=listener";
  cfg.codec.name = "h264";
  cfg.codec.encoder = "openh264enc";
  cfg.codec.bitrate_kbps = 1800;
  cfg.codec.gop = 45;

  auto pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("openh264enc"), std::string::npos);
  EXPECT_NE(pipeline.find("gop-size=45"), std::string::npos);
  EXPECT_NE(pipeline.find("bitrate=1800000"), std::string::npos);

  cfg.codec.encoder = "vaapih264enc";
  pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("vaapih264enc"), std::string::npos);
  EXPECT_NE(pipeline.find("video/x-raw,format=NV12"), std::string::npos);
  EXPECT_NE(pipeline.find("keyframe-period=45"), std::string::npos);
}

TEST(PipelineBuilderTest, BuildsAv1HardwareAndAlternativeSoftwareChains) {
  ros2_gst_video_bridge::GstBridgeConfig cfg;
  cfg.transport.kind = "srt";
  cfg.transport.sink_uri = "srt://127.0.0.1:9000?mode=listener";
  cfg.codec.name = "av1";
  cfg.codec.encoder = "nvv4l2av1enc";
  cfg.codec.bitrate_kbps = 2400;
  cfg.codec.gop = 36;

  auto pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("nvv4l2av1enc"), std::string::npos);
  EXPECT_NE(pipeline.find("nvvidconv"), std::string::npos);
  EXPECT_NE(pipeline.find("video/x-raw(memory:NVMM),format=NV12"), std::string::npos);
  EXPECT_NE(pipeline.find("bitrate=2400000"), std::string::npos);
  EXPECT_NE(pipeline.find("iframeinterval=36"), std::string::npos);
  EXPECT_NE(pipeline.find("av1parse"), std::string::npos);

  cfg.codec.encoder = "vaav1enc";
  pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("vaav1enc"), std::string::npos);
  EXPECT_NE(pipeline.find("video/x-raw,format=NV12"), std::string::npos);
  EXPECT_NE(pipeline.find("keyframe-period=36"), std::string::npos);

  cfg.codec.encoder = "rav1enc";
  pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("rav1enc"), std::string::npos);
  EXPECT_NE(pipeline.find("video/x-raw,format=I420"), std::string::npos);
  EXPECT_NE(pipeline.find("av1parse"), std::string::npos);
}

TEST(PipelineBuilderTest, InsertsAutomaticDebayerForDetectedBayerInput) {
  ros2_gst_video_bridge::GstBridgeConfig cfg;
  cfg.transport.kind = "srt";
  cfg.transport.sink_uri = "srt://127.0.0.1:9000?mode=listener";
  cfg.codec.name = "h265";
  cfg.codec.encoder = "nvv4l2h265enc";
  cfg.source.detected_bayer_format = "rggb";

  const auto pipeline = ros2_gst_video_bridge::PipelineBuilder::build(cfg);
  EXPECT_NE(pipeline.find("bayer2rgb ! videoconvert !"), std::string::npos);
  EXPECT_NE(pipeline.find("nvv4l2h265enc"), std::string::npos);
}

} // namespace
