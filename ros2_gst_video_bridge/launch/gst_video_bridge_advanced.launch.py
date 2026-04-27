from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    params_file = LaunchConfiguration('params_file')
    input_topic = LaunchConfiguration('input_topic')
    enable_debayer = LaunchConfiguration('enable_debayer')
    debayer_output_topic = LaunchConfiguration('debayer_output_topic')
    bridge_input_topic = PythonExpression([
        "'", debayer_output_topic, "' if '", enable_debayer,
        "' == 'true' else '", input_topic, "'",
    ])

    return LaunchDescription([
        DeclareLaunchArgument(
            'params_file',
            default_value=PathJoinSubstitution([
                FindPackageShare('ros2_gst_video_bridge'),
                'config',
                'default_params.yaml',
            ]),
            description='YAML parameter file with full transport/codec/runtime overrides',
        ),
        DeclareLaunchArgument('profile_machine', default_value='generic'),
        DeclareLaunchArgument('profile_stream', default_value='low_latency'),
        DeclareLaunchArgument('input_topic', default_value='/camera/image_raw'),
        DeclareLaunchArgument('enable_debayer', default_value='false'),
        DeclareLaunchArgument('debayer_output_topic', default_value='/camera/image_color'),
        DeclareLaunchArgument('transport_kind', default_value='srt'),
        DeclareLaunchArgument('sink_uri', default_value='srt://127.0.0.1:9000?mode=listener'),
        DeclareLaunchArgument('transport_latency_ms', default_value='60'),
        DeclareLaunchArgument('reconnect_enabled', default_value='true'),
        DeclareLaunchArgument('reconnect_interval_ms', default_value='1000'),
        DeclareLaunchArgument('reconnect_max_attempts', default_value='0'),
        DeclareLaunchArgument('codec_name', default_value='auto'),
        DeclareLaunchArgument('codec_encoder', default_value=''),
        DeclareLaunchArgument('codec_profile', default_value='baseline'),
        DeclareLaunchArgument('codec_tune', default_value='zerolatency'),
        DeclareLaunchArgument('codec_rate_control', default_value='cbr'),
        DeclareLaunchArgument('codec_bitrate_kbps', default_value='2000'),
        DeclareLaunchArgument('codec_gop', default_value='30'),
        DeclareLaunchArgument('max_fps', default_value='30.0'),
        DeclareLaunchArgument('use_wall_clock_timestamps', default_value='false'),
        DeclareLaunchArgument('runtime_mode', default_value='stream'),
        DeclareLaunchArgument('print_effective_config', default_value='true'),
        DeclareLaunchArgument('pipeline_override', default_value=''),
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
            parameters=[
                params_file,
                {
                    'profile.machine': LaunchConfiguration('profile_machine'),
                    'profile.stream': LaunchConfiguration('profile_stream'),
                    'input_topic': bridge_input_topic,
                    'transport.kind': LaunchConfiguration('transport_kind'),
                    'transport.sink_uri': LaunchConfiguration('sink_uri'),
                    'transport.latency_ms': LaunchConfiguration('transport_latency_ms'),
                    'transport.reconnect.enabled': LaunchConfiguration('reconnect_enabled'),
                    'transport.reconnect.interval_ms': LaunchConfiguration(
                        'reconnect_interval_ms'
                    ),
                    'transport.reconnect.max_attempts': LaunchConfiguration(
                        'reconnect_max_attempts'
                    ),
                    'codec.name': LaunchConfiguration('codec_name'),
                    'codec.encoder': LaunchConfiguration('codec_encoder'),
                    'codec.profile': LaunchConfiguration('codec_profile'),
                    'codec.tune': LaunchConfiguration('codec_tune'),
                    'codec.rate_control': LaunchConfiguration('codec_rate_control'),
                    'codec.bitrate_kbps': LaunchConfiguration('codec_bitrate_kbps'),
                    'codec.gop': LaunchConfiguration('codec_gop'),
                    'max_fps': LaunchConfiguration('max_fps'),
                    'use_wall_clock_timestamps': LaunchConfiguration('use_wall_clock_timestamps'),
                    'runtime.mode': LaunchConfiguration('runtime_mode'),
                    'runtime.print_effective_config': LaunchConfiguration(
                        'print_effective_config'
                    ),
                    'gst.pipeline_override': LaunchConfiguration('pipeline_override'),
                },
            ],
        ),
    ])
