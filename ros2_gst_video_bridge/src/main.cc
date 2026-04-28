// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include <rclcpp/rclcpp.hpp>

#include "ros2_gst_video_bridge/gst_video_bridge_node.hpp"

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ros2_gst_video_bridge::GstVideoBridgeNode>();

    if (node->hasImmediateExit()) {
        const int code = node->immediateExitCode();
        rclcpp::shutdown();
        return code;
    }

    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
