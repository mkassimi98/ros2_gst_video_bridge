from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch_ros.actions import Node


def generate_launch_description():
    profile_machine = LaunchConfiguration('profile_machine')
    profile_stream = LaunchConfiguration('profile_stream')
    input_topic = LaunchConfiguration('input_topic')
    enable_debayer = LaunchConfiguration('enable_debayer')
    debayer_output_topic = LaunchConfiguration('debayer_output_topic')
    sink_uri = LaunchConfiguration('sink_uri')
    codec_name = LaunchConfiguration('codec_name')
    codec_encoder = LaunchConfiguration('codec_encoder')
    runtime_mode = LaunchConfiguration('runtime_mode')
    bridge_input_topic = PythonExpression([
        "'", debayer_output_topic, "' if '", enable_debayer,
        "' == 'true' else '", input_topic, "'",
    ])

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
            'enable_debayer',
            default_value='false',
            description='Enable image_proc debayer node before bridge (true|false)',
        ),
        DeclareLaunchArgument(
            'debayer_output_topic',
            default_value='/camera/image_color',
            description='Debayered image topic used by the bridge when enable_debayer=true',
        ),
        DeclareLaunchArgument(
            'sink_uri',
            default_value='srt://127.0.0.1:9000?mode=listener',
            description='Transport URI (SRT/UDP/RTSP/file depending on profile/transport)',
        ),
        DeclareLaunchArgument(
            'codec_name',
            default_value='auto',
            description='Codec selection: auto|av1|h265|h264|mjpeg',
        ),
        DeclareLaunchArgument(
            'codec_encoder',
            default_value='',
            description='Optional GStreamer encoder override, e.g. nvv4l2h265enc',
        ),
        DeclareLaunchArgument(
            'runtime_mode',
            default_value='stream',
            description='Runtime mode: stream|list_topics|list_capabilities|validate_config|discover',
        ),
        Node(
            package='image_proc',
            executable='debayer_node',
            name='gst_video_bridge_debayer',
            output='screen',
            condition=IfCondition(enable_debayer),
            remappings=[
                ('image_raw', input_topic),
                ('image_color', debayer_output_topic),
            ],
        ),
        Node(
            package='ros2_gst_video_bridge',
            executable='gst_video_bridge_node',
            name='gst_video_bridge',
            output='screen',
            parameters=[{
                'profile.machine': profile_machine,
                'profile.stream': profile_stream,
                'input_topic': bridge_input_topic,
                'codec.name': codec_name,
                'codec.encoder': codec_encoder,
                'transport.sink_uri': sink_uri,
                'runtime.mode': runtime_mode,
            }],
        ),
    ])
