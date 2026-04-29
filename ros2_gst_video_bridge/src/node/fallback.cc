// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/core/pipeline_builder.hpp"
#include "ros2_gst_video_bridge/detail/encoder_selection.hpp"
#include "ros2_gst_video_bridge/gst_video_bridge_node.hpp"

namespace ros2_gst_video_bridge {

namespace {} // namespace

std::string GstVideoBridgeNode::selectSoftwareEncoderForCodec() const
{
    return detail::selectSoftwareEncoderForCodec(config_.codec.name, encoder_implementations_);
}

bool GstVideoBridgeNode::requestSoftwareFallback(const std::string& reason)
{
    if (!auto_codec_requested_ || !hw_encoder_selected_ || sw_fallback_applied_) {
        return false;
    }

    ++consecutive_stream_failures_;
    if (consecutive_stream_failures_ < hw_fallback_failure_threshold_) {
        return false;
    }

    const std::string sw_encoder = selectSoftwareEncoderForCodec();
    if (sw_encoder.empty()) {
        RCLCPP_WARN(
                this->get_logger(),
                "runtime fallback requested after repeated failures but no SW encoder was found "
                "for codec '%s'",
                config_.codec.name.c_str());
        return false;
    }

    config_.codec.encoder  = sw_encoder;
    effective_pipeline_    = PipelineBuilder::build(config_);
    sw_fallback_applied_   = true;
    sw_fallback_requested_ = true;

    if (metrics_publisher_) {
        metrics_publisher_->setCodecSelectionFlags(auto_codec_requested_, hw_encoder_selected_, sw_fallback_applied_);
        metrics_publisher_->setSessionMetadata(
                session_id_,
                stream_id_,
                config_.source.input_topic,
                config_.codec.name,
                config_.codec.encoder,
                config_.transport.kind,
                config_.transport.sink_uri);
    }

    RCLCPP_WARN(
            this->get_logger(),
            "Switching encoder fallback from HW to SW after %d failures. codec=%s hw_reason=%s "
            "sw_encoder=%s",
            consecutive_stream_failures_,
            config_.codec.name.c_str(),
            reason.c_str(),
            sw_encoder.c_str());
    publishRuntimeEvent("warning", "ENCODER_FALLBACK_SW", reason);
    return true;
}

} // namespace ros2_gst_video_bridge
