// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#ifndef ROS2_GST_VIDEO_BRIDGE__GST_VIDEO_BRIDGE_NODE_HPP_
#define ROS2_GST_VIDEO_BRIDGE__GST_VIDEO_BRIDGE_NODE_HPP_

#include <chrono>
#include <cstdint>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <ros2_gst_video_bridge_msgs/srv/set_streaming_profile.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <string>
#include <vector>

#include "ros2_gst_video_bridge/core/gst_bridge_config.hpp"
#include "ros2_gst_video_bridge/runtime/capability_probe.hpp"
#include "ros2_gst_video_bridge/runtime/metrics_publisher.hpp"
#include "ros2_gst_video_bridge/runtime/stream_engine.hpp"
#include "ros2_gst_video_bridge/runtime/topic_introspector.hpp"

namespace ros2_gst_video_bridge {

class GstVideoBridgeNode : public rclcpp::Node
{
  public:
    explicit GstVideoBridgeNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());
    ~GstVideoBridgeNode() override;

    bool hasImmediateExit() const;
    int  immediateExitCode() const;

  private:
    enum class RuntimeState : uint8_t {
        Connecting,
        Streaming,
        Degraded,
        Reconnecting,
        Failed
    };

    bool handleRuntimeMode();
    void printImageTopics() const;
    void printGstCapabilities() const;
    bool validateConfiguration() const;

    void               initializeModules();
    void               initializeSubscriptions();
    void               initializeHealthMonitoring();
    void               onImage(const sensor_msgs::msg::Image::SharedPtr msg);
    void               runHealthCheck();
    void               evaluateAdaptationPolicy();
    void               applyAdaptationLevel(uint8_t level, const std::string& reason);
    bool               requestPipelineReconfigure(const std::string& reason);
    void               scheduleReconnect(const std::string& reason, bool allow_software_fallback = true);
    bool               tryReconnect();
    bool               requestSoftwareFallback(const std::string& reason);
    std::string        selectSoftwareEncoderForCodec() const;
    void               setRuntimeState(RuntimeState new_state, const std::string& reason = "");
    static const char* runtimeStateToString(RuntimeState state);

    void logConfiguration() const;
    void logRuntimeCapabilities() const;
    void publishRuntimeEvent(const std::string& severity, const std::string& code, const std::string& message) const;
    void publishFirstFailureSnapshot(const std::string& reason);
    void handleSetStreamingProfile(
            const std::shared_ptr<ros2_gst_video_bridge_msgs::srv::SetStreamingProfile::Request> request,
            std::shared_ptr<ros2_gst_video_bridge_msgs::srv::SetStreamingProfile::Response>      response);
    static bool isValidAdaptationProfile(const std::string& profile);
    std::string buildSessionId() const;

    GstBridgeConfig config_;
    std::string     effective_pipeline_;
    int             immediate_exit_code_{-1};

    std::unique_ptr<TopicIntrospector>                                               topic_introspector_;
    std::unique_ptr<CapabilityProbe>                                                 capability_probe_;
    std::unique_ptr<StreamEngine>                                                    stream_engine_;
    std::unique_ptr<MetricsPublisher>                                                metrics_publisher_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr                         image_subscription_;
    rclcpp::Service<ros2_gst_video_bridge_msgs::srv::SetStreamingProfile>::SharedPtr set_profile_service_;
    rclcpp::TimerBase::SharedPtr                                                     health_timer_;

    std::chrono::steady_clock::time_point last_throttle_steady_time_{};
    std::chrono::steady_clock::time_point last_reconnect_attempt_{};
    std::chrono::steady_clock::time_point backpressure_started_at_{};
    RuntimeState                          runtime_state_{RuntimeState::Failed};
    bool                                  reconnect_requested_{false};
    int                                   reconnect_attempts_{0};
    uint64_t                              reconnect_count_{0};
    int                                   consecutive_backpressure_drops_{0};
    double                                throttle_tokens_{1.0};

    std::string session_id_;
    std::string stream_id_;

    std::vector<std::string> encoder_implementations_;
    bool                     auto_codec_requested_{false};
    bool                     hw_encoder_selected_{false};
    bool                     sw_fallback_applied_{false};
    bool                     sw_fallback_requested_{false};
    int                      consecutive_stream_failures_{0};
    int                      hw_fallback_failure_threshold_{3};

    bool        adaptation_enabled_{true};
    int         adaptation_interval_ms_{2000};
    int         adaptation_cooldown_ms_{5000};
    std::string adaptation_profile_{"balanced"};
    uint8_t     adaptation_level_{0};
    uint64_t    last_adaptation_ns_{0};

    int    base_bitrate_kbps_{2000};
    int    base_gop_{30};
    double base_max_fps_{30.0};

    bool pipeline_reconfigure_requested_{false};
    bool first_failure_snapshot_published_{false};
};

} // namespace ros2_gst_video_bridge

#endif // ROS2_GST_VIDEO_BRIDGE__GST_VIDEO_BRIDGE_NODE_HPP_
