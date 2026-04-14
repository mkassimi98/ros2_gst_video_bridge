from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    profile_machine = LaunchConfiguration('profile_machine')
    profile_stream = LaunchConfiguration('profile_stream')
    input_topic = LaunchConfiguration('input_topic')
    sink_uri = LaunchConfiguration('sink_uri')

    return LaunchDescription([
        DeclareLaunchArgument(
            'profile_machine',
            default_value='generic',
            description='Machine profile: generic|jetson|x86|raspi',
        ),
        DeclareLaunchArgument(
            'profile_stream',
            default_value='low_latency',
            description=(
                'Stream profile: '
                'default|low_latency|low_bandwidth|high_quality|monitoring_udp'
            ),
        ),
        DeclareLaunchArgument(
            'input_topic',
            default_value='/camera/image_raw',
            description='ROS image input topic',
        ),
        DeclareLaunchArgument(
            'sink_uri',
            default_value='srt://127.0.0.1:9000?mode=listener',
            description='Transport URI (SRT/UDP/RTSP/file depending on profile/transport)',
        ),
        Node(
            package='ros2_gst_video_bridge',
            executable='gst_video_bridge_node',
            name='gst_video_bridge',
            output='screen',
            parameters=[{
                'profile.machine': profile_machine,
                'profile.stream': profile_stream,
                'input_topic': input_topic,
                'transport.sink_uri': sink_uri,
                'runtime.mode': 'stream',
            }],
        ),
    ])
