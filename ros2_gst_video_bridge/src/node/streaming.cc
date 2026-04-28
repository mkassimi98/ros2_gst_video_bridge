// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>

#include "ros2_gst_video_bridge/core/pipeline_builder.hpp"
#include "ros2_gst_video_bridge/gst_video_bridge_node.hpp"
#include "ros2_gst_video_bridge/runtime/image_format.hpp"

namespace ros2_gst_video_bridge {

namespace {

consteval int64_t nanosecondsPerSecond()
{
    return 1000000000LL;
}

bool isBackpressureOnlyError(const std::string& error)
{
    return error.rfind("appsrc queue full", 0) == 0;
}

constexpr double kThrottleBurstWindowSeconds = 0.25;
constexpr double kThrottleTokenTolerance     = 0.02;

std::string buildBackpressureReconnectReason(int consecutive_drops, int elapsed_ms)
{
    std::ostringstream ss;
    ss << "sustained appsrc backpressure: consecutive_drops=" << consecutive_drops << " elapsed_ms=" << elapsed_ms;
    return ss.str();
}

} // namespace

void GstVideoBridgeNode::initializeModules()
{
    topic_introspector_ = std::make_unique<TopicIntrospector>(*this);
    capability_probe_   = std::make_unique<CapabilityProbe>();
    stream_engine_      = std::make_unique<StreamEngine>(effective_pipeline_);
    metrics_publisher_  = std::make_unique<MetricsPublisher>(*this);

    metrics_publisher_->setSessionMetadata(
            session_id_,
            stream_id_,
            config_.source.input_topic,
            config_.codec.name,
            config_.codec.encoder,
            config_.transport.kind,
            config_.transport.sink_uri);
    metrics_publisher_->setCodecSelectionFlags(auto_codec_requested_, hw_encoder_selected_, sw_fallback_applied_);
    metrics_publisher_->setEffectiveEncoding(
            static_cast<uint32_t>(config_.codec.bitrate_kbps),
            static_cast<uint32_t>(config_.codec.gop),
            static_cast<float>(config_.runtime.max_fps));
    metrics_publisher_->setAdaptationState(adaptation_profile_, adaptation_level_);

    set_profile_service_ = this->create_service<ros2_gst_video_bridge_msgs::srv::SetStreamingProfile>(
            "~/set_streaming_profile",
            std::bind(
                    &GstVideoBridgeNode::handleSetStreamingProfile,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2));

    if (!stream_engine_->start()) {
        const auto initial_error = stream_engine_->lastError();
        if (requestSoftwareFallback(initial_error)) {
            stream_engine_ = std::make_unique<StreamEngine>(effective_pipeline_);
            if (stream_engine_->start()) {
                setRuntimeState(RuntimeState::Connecting, "pipeline restarted with software fallback");
                metrics_publisher_->publishHeartbeat();
                return;
            }
        }

        RCLCPP_ERROR(this->get_logger(), "Failed to start stream engine: %s", stream_engine_->lastError().c_str());
        publishRuntimeEvent("error", "PIPELINE_START_FAILED", stream_engine_->lastError());
        publishFirstFailureSnapshot(stream_engine_->lastError());

        if (config_.transport.reconnect_enabled) {
            setRuntimeState(RuntimeState::Degraded, stream_engine_->lastError());
            reconnect_requested_ = true;
        } else {
            setRuntimeState(RuntimeState::Failed, stream_engine_->lastError());
            immediate_exit_code_ = 1;
        }
        return;
    }

    setRuntimeState(RuntimeState::Connecting, "pipeline started");
    publishRuntimeEvent("info", "PIPELINE_STARTED", "Pipeline transitioned to PLAYING");
    metrics_publisher_->publishHeartbeat();
}

void GstVideoBridgeNode::initializeSubscriptions()
{
    if (immediate_exit_code_ >= 0) {
        return;
    }

    image_subscription_ = this->create_subscription<sensor_msgs::msg::Image>(
            config_.source.input_topic,
            rclcpp::SensorDataQoS(),
            std::bind(&GstVideoBridgeNode::onImage, this, std::placeholders::_1));
}

void GstVideoBridgeNode::initializeHealthMonitoring()
{
    if (immediate_exit_code_ >= 0) {
        return;
    }

    health_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(500), std::bind(&GstVideoBridgeNode::runHealthCheck, this));
}

void GstVideoBridgeNode::onImage(const sensor_msgs::msg::Image::SharedPtr msg)
{
    if (!stream_engine_ || !stream_engine_->isRunning() || pipeline_reconfigure_requested_) {
        return;
    }

    const auto validation = validateImageShape(*msg);
    if (!validation.valid) {
        if (metrics_publisher_) {
            metrics_publisher_->recordFrameDroppedMalformed();
        }
        RCLCPP_WARN_THROTTLE(
                this->get_logger(), *this->get_clock(), 5000, "Dropping image: %s", validation.error.c_str());
        return;
    }

    uint64_t timestamp_ns = 0;
    if (config_.runtime.use_wall_clock_timestamps) {
        timestamp_ns = static_cast<uint64_t>(this->now().nanoseconds());
    } else {
        timestamp_ns = static_cast<uint64_t>(
                static_cast<int64_t>(msg->header.stamp.sec) * nanosecondsPerSecond() + msg->header.stamp.nanosec);
        if (timestamp_ns == 0) {
            timestamp_ns = static_cast<uint64_t>(this->now().nanoseconds());
        }
    }

    if (metrics_publisher_) {
        const uint64_t now_ns = static_cast<uint64_t>(this->now().nanoseconds());
        metrics_publisher_->recordFrameIn(now_ns, timestamp_ns);
    }

    if (config_.runtime.max_fps > 0.0) {
        const auto   now            = std::chrono::steady_clock::now();
        const double token_capacity = std::max(1.0, config_.runtime.max_fps * kThrottleBurstWindowSeconds);

        if (last_throttle_steady_time_.time_since_epoch().count() == 0) {
            throttle_tokens_ = 1.0;
        } else {
            const double elapsed_s = std::chrono::duration<double>(now - last_throttle_steady_time_).count();
            throttle_tokens_       = std::min(token_capacity, throttle_tokens_ + elapsed_s * config_.runtime.max_fps);
        }
        last_throttle_steady_time_ = now;

        if (throttle_tokens_ + kThrottleTokenTolerance < 1.0) {
            if (metrics_publisher_) {
                metrics_publisher_->recordFrameDroppedThrottle();
            }
            return;
        }
        throttle_tokens_ = std::max(0.0, throttle_tokens_ - 1.0);
    }

    std::string gst_format;
    if (isBayerEncoding(msg->encoding)) {
        const std::string bayer_format = resolveGstBayerFormat(msg->encoding);
        if (!bayer_format.empty() && config_.source.auto_debayer) {
            if (config_.source.detected_bayer_format != bayer_format) {
                config_.source.detected_bayer_format = bayer_format;
                RCLCPP_INFO(
                        this->get_logger(),
                        "Detected Bayer input '%s'; enabling automatic in-pipeline debayer.",
                        msg->encoding.c_str());
                (void)requestPipelineReconfigure(
                        "automatic Bayer debayer enabled for input encoding '" + msg->encoding + "'");
                return;
            }
            gst_format = bayer_format;
        }
    }

    if (gst_format.empty() && !resolveGstFormat(msg->encoding, gst_format)) {
        RCLCPP_WARN_THROTTLE(
                this->get_logger(),
                *this->get_clock(),
                5000,
                "Unsupported image encoding '%s'. Supported encodings include mono8/mono16, rgb8/bgr8, "
                "rgba8/bgra8, type_8/16 C1/C3/C4, and bayer_*8/*16 (grayscale fallback).",
                msg->encoding.c_str());
        return;
    }

    if (isBayerEncoding(msg->encoding) && config_.source.detected_bayer_format.empty()) {
        RCLCPP_WARN_THROTTLE(
                this->get_logger(),
                *this->get_clock(),
                10000,
                "Input encoding '%s' is being streamed as grayscale Bayer fallback. "
                "Automatic in-pipeline debayer is only available for 8-bit Bayer.",
                msg->encoding.c_str());
    }

    const auto push_begin = std::chrono::steady_clock::now();
    const bool pushed     = stream_engine_->pushFrame(
            msg->data.data(),
            msg->data.size(),
            static_cast<int>(msg->width),
            static_cast<int>(msg->height),
            gst_format,
            timestamp_ns);
    const auto   push_end        = std::chrono::steady_clock::now();
    const double push_latency_ms = std::chrono::duration<double, std::milli>(push_end - push_begin).count();

    if (metrics_publisher_) {
        metrics_publisher_->recordPushLatency(push_latency_ms);
    }

    if (!pushed) {
        if (metrics_publisher_) {
            metrics_publisher_->recordFrameDroppedBackpressure();
        }
        const auto error = stream_engine_->lastError();
        if (isBackpressureOnlyError(error)) {
            const auto now = std::chrono::steady_clock::now();
            if (backpressure_started_at_.time_since_epoch().count() == 0) {
                backpressure_started_at_ = now;
            }
            ++consecutive_backpressure_drops_;

            const auto elapsed_ms = static_cast<int>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(now - backpressure_started_at_).count());
            const bool timed_out = elapsed_ms >= config_.runtime.backpressure_reconnect_after_ms;
            const bool too_many_drops
                    = consecutive_backpressure_drops_ >= config_.runtime.backpressure_max_consecutive_drops;

            if (timed_out || too_many_drops) {
                const auto reason = buildBackpressureReconnectReason(consecutive_backpressure_drops_, elapsed_ms);
                backpressure_started_at_        = std::chrono::steady_clock::time_point{};
                consecutive_backpressure_drops_ = 0;
                scheduleReconnect(reason, false);
            }
        } else {
            backpressure_started_at_        = std::chrono::steady_clock::time_point{};
            consecutive_backpressure_drops_ = 0;
            scheduleReconnect(error);
        }
        RCLCPP_WARN_THROTTLE(
                this->get_logger(), *this->get_clock(), 2000, "Failed to push frame to GStreamer: %s", error.c_str());
    } else {
        if (metrics_publisher_) {
            metrics_publisher_->recordFrameOut();
        }
        consecutive_stream_failures_    = 0;
        consecutive_backpressure_drops_ = 0;
        backpressure_started_at_        = std::chrono::steady_clock::time_point{};
    }
}

} // namespace ros2_gst_video_bridge
