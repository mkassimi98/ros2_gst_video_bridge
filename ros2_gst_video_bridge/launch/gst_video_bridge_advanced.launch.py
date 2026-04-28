from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def _lc(name):
    return LaunchConfiguration(name)


def _bool_param(name):
    return ParameterValue(_lc(name), value_type=bool)


def _int_param(name):
    return ParameterValue(_lc(name), value_type=int)


def _float_param(name):
    return ParameterValue(_lc(name), value_type=float)


def _arg(name, default_value, description):
    return DeclareLaunchArgument(
        name,
        default_value=default_value,
        description=description,
    )


COMMON_ARGUMENTS = (
    ('profile_machine', 'generic', 'Machine profile: generic|jetson|x86|raspi'),
    (
        'profile_stream',
        'low_latency',
        'Stream profile: default|low_latency|low_bandwidth|high_quality|monitoring_udp',
    ),
    ('input_topic', '/camera/image_raw', 'ROS image input topic'),
    ('enable_debayer', 'false', 'Enable image_proc debayer node before bridge (true|false)'),
    (
        'debayer_output_topic',
        '/camera/image_color',
        'Debayered image topic used by the bridge when enable_debayer=true',
    ),
    (
        'transport_kind',
        'srt',
        'Transport backend: srt|udp|rtsp|file',
    ),
    (
        'sink_uri',
        'srt://127.0.0.1:9000?mode=listener',
        'Transport URI (SRT/UDP/RTSP/file depending on profile/transport)',
    ),
    ('transport_latency_ms', '60', 'Transport buffering/latency in milliseconds'),
    ('reconnect_enabled', 'true', 'Enable transport reconnection policy (true|false)'),
    ('reconnect_interval_ms', '1000', 'Delay between reconnect attempts in milliseconds'),
    ('reconnect_max_attempts', '0', 'Maximum reconnect attempts; 0 means unlimited'),
    ('codec_name', 'auto', 'Codec selection: auto|av1|h265|h264|mjpeg'),
    ('codec_encoder', '', 'Optional GStreamer encoder override, e.g. nvv4l2h265enc'),
    ('codec_profile', 'baseline', 'Encoder profile when supported by the selected codec'),
    ('codec_tune', 'zerolatency', 'Encoder tune/preset intent when supported'),
    ('codec_rate_control', 'cbr', 'Rate-control mode, for example cbr|vbr'),
    ('codec_bitrate_kbps', '2000', 'Target video bitrate in kbit/s'),
    ('codec_gop', '30', 'GOP/keyframe interval in frames'),
    ('max_fps', '30.0', 'Maximum output frame rate; 0 disables throttling'),
    (
        'use_wall_clock_timestamps',
        'false',
        'Use wall-clock timestamps instead of ROS message timestamps',
    ),
    (
        'runtime_mode',
        'stream',
        'Runtime mode: stream|list_topics|list_capabilities|validate_config|discover',
    ),
    ('print_effective_config', 'true', 'Print the resolved configuration on startup'),
    ('stream_id', 'default', 'Logical stream identifier used by runtime state and metrics'),
    ('hw_fallback_failures', '3', 'Hardware encoder failures before falling back to CPU'),
    ('adaptation_enabled', 'true', 'Enable runtime bitrate/fps adaptation (true|false)'),
    (
        'adaptation_profile',
        'balanced',
        'Runtime adaptation profile: conservative|balanced|aggressive',
    ),
    (
        'adaptation_interval_ms',
        '2000',
        'Runtime adaptation evaluation interval in milliseconds',
    ),
    (
        'adaptation_cooldown_ms',
        '5000',
        'Cooldown after an adaptation step in milliseconds',
    ),
    (
        'pipeline_override',
        '',
        'Full GStreamer pipeline override; empty uses generated pipeline',
    ),
)


def generate_launch_description():
    params_file = _lc('params_file')
    input_topic = _lc('input_topic')
    enable_debayer = _lc('enable_debayer')
    debayer_output_topic = _lc('debayer_output_topic')
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
        *[_arg(*argument) for argument in COMMON_ARGUMENTS],
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
                    'runtime.stream_id': _lc('stream_id'),
                    'runtime.hw_fallback_failures': _int_param('hw_fallback_failures'),
                    'runtime.adaptation.enabled': _bool_param('adaptation_enabled'),
                    'runtime.adaptation.profile': _lc('adaptation_profile'),
                    'runtime.adaptation.interval_ms': _int_param('adaptation_interval_ms'),
                    'runtime.adaptation.cooldown_ms': _int_param('adaptation_cooldown_ms'),
                    'gst.pipeline_override': _lc('pipeline_override'),
                },
            ],
        ),
    ])
