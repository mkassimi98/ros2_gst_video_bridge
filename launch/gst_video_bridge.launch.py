from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='ros2_gst_video_bridge',
            executable='gst_video_bridge_node',
            name='gst_video_bridge',
            output='screen',
            parameters=['config/default_params.yaml'],
        )
    ])
