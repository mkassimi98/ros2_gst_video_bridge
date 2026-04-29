from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def _lc(name):
    return LaunchConfiguration(name)


def _bool_param(name):
    return ParameterValue(_lc(name), value_type=bool)


def _int_param(name):
    return ParameterValue(_lc(name), value_type=int)


def _float_param(name):
    return ParameterValue(_lc(name), value_type=float)


def generate_launch_description():
    input_topic = _lc('input_topic')
    enable_debayer = _lc('enable_debayer')
    debayer_output_topic = _lc('debayer_output_topic')
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
            'transport_kind',
            default_value='srt',
            description='Transport backend: srt|udp|rtsp|file',
        ),
        DeclareLaunchArgument(
            'transport_latency_ms',
            default_value='60',
            description='Transport buffering/latency in milliseconds',
        ),
        DeclareLaunchArgument(
            'reconnect_enabled',
            default_value='true',
            description='Enable transport reconnection policy (true|false)',
        ),
        DeclareLaunchArgument(
            'reconnect_interval_ms',
            default_value='1000',
            description='Delay between reconnect attempts in milliseconds',
        ),
        DeclareLaunchArgument(
            'reconnect_max_attempts',
            default_value='0',
            description='Maximum reconnect attempts; 0 means unlimited',
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
            'codec_profile',
            default_value='baseline',
            description='Encoder profile when supported by the selected codec',
        ),
        DeclareLaunchArgument(
            'codec_tune',
            default_value='zerolatency',
            description='Encoder tune/preset intent when supported',
        ),
        DeclareLaunchArgument(
            'codec_rate_control',
            default_value='cbr',
            description='Rate-control mode, for example cbr|vbr',
        ),
        DeclareLaunchArgument(
            'codec_bitrate_kbps',
            default_value='2000',
            description='Target video bitrate in kbit/s',
        ),
        DeclareLaunchArgument(
            'codec_gop',
            default_value='30',
            description='GOP/keyframe interval in frames',
        ),
        DeclareLaunchArgument(
            'max_fps',
            default_value='30.0',
            description='Maximum output frame rate; 0 disables throttling',
        ),
        DeclareLaunchArgument(
            'use_wall_clock_timestamps',
            default_value='false',
            description='Use wall-clock timestamps instead of ROS message timestamps',
        ),
        DeclareLaunchArgument(
            'runtime_mode',
            default_value='stream',
            description=(
                'Runtime mode: '
                'stream|list_topics|list_capabilities|validate_config|discover'
            ),
        ),
        DeclareLaunchArgument(
            'print_effective_config',
            default_value='true',
            description='Print the resolved configuration on startup',
        ),
        DeclareLaunchArgument(
            'backpressure_reconnect_after_ms',
            default_value='2000',
            description='Reconnect when appsrc backpressure lasts this many milliseconds',
        ),
        DeclareLaunchArgument(
            'backpressure_max_consecutive_drops',
            default_value='60',
            description='Reconnect after this many consecutive appsrc backpressure drops',
        ),
        DeclareLaunchArgument(
            'stream_id',
            default_value='default',
            description='Logical stream identifier used by runtime state and metrics',
        ),
        DeclareLaunchArgument(
            'hw_fallback_failures',
            default_value='3',
            description='Hardware encoder failures before falling back to CPU',
        ),
        DeclareLaunchArgument(
            'adaptation_enabled',
            default_value='true',
            description='Enable runtime bitrate/fps adaptation (true|false)',
        ),
        DeclareLaunchArgument(
            'adaptation_profile',
            default_value='balanced',
            description='Runtime adaptation profile: conservative|balanced|aggressive',
        ),
        DeclareLaunchArgument(
            'adaptation_interval_ms',
            default_value='2000',
            description='Runtime adaptation evaluation interval in milliseconds',
        ),
        DeclareLaunchArgument(
            'adaptation_cooldown_ms',
            default_value='5000',
            description='Cooldown after an adaptation step in milliseconds',
        ),
        DeclareLaunchArgument(
            'pipeline_override',
            default_value='',
            description='Full GStreamer pipeline override; empty uses generated pipeline',
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
                'profile.machine': _lc('profile_machine'),
                'profile.stream': _lc('profile_stream'),
                'input_topic': bridge_input_topic,
                'transport.kind': _lc('transport_kind'),
                'transport.sink_uri': _lc('sink_uri'),
                'transport.latency_ms': _int_param('transport_latency_ms'),
                'transport.reconnect.enabled': _bool_param('reconnect_enabled'),
                'transport.reconnect.interval_ms': _int_param('reconnect_interval_ms'),
                'transport.reconnect.max_attempts': _int_param('reconnect_max_attempts'),
                'codec.name': _lc('codec_name'),
                'codec.encoder': _lc('codec_encoder'),
                'codec.profile': _lc('codec_profile'),
                'codec.tune': _lc('codec_tune'),
                'codec.rate_control': _lc('codec_rate_control'),
                'codec.bitrate_kbps': _int_param('codec_bitrate_kbps'),
                'codec.gop': _int_param('codec_gop'),
                'max_fps': _float_param('max_fps'),
                'use_wall_clock_timestamps': _bool_param('use_wall_clock_timestamps'),
                'runtime.mode': _lc('runtime_mode'),
                'runtime.print_effective_config': _bool_param('print_effective_config'),
                'runtime.backpressure.reconnect_after_ms': _int_param(
                    'backpressure_reconnect_after_ms'
                ),
                'runtime.backpressure.max_consecutive_drops': _int_param(
                    'backpressure_max_consecutive_drops'
                ),
                'runtime.stream_id': _lc('stream_id'),
                'runtime.hw_fallback_failures': _int_param('hw_fallback_failures'),
                'runtime.adaptation.enabled': _bool_param('adaptation_enabled'),
                'runtime.adaptation.profile': _lc('adaptation_profile'),
                'runtime.adaptation.interval_ms': _int_param('adaptation_interval_ms'),
                'runtime.adaptation.cooldown_ms': _int_param('adaptation_cooldown_ms'),
                'gst.pipeline_override': _lc('pipeline_override'),
            }],
        ),
    ])
