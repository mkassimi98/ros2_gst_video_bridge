#!/usr/bin/env python3
import re
import subprocess
import sys


COMMON_ARGUMENTS = {
    'profile_machine',
    'profile_stream',
    'input_topic',
    'enable_debayer',
    'debayer_output_topic',
    'transport_kind',
    'sink_uri',
    'transport_latency_ms',
    'reconnect_enabled',
    'reconnect_interval_ms',
    'reconnect_max_attempts',
    'codec_name',
    'codec_encoder',
    'codec_profile',
    'codec_tune',
    'codec_rate_control',
    'codec_bitrate_kbps',
    'codec_gop',
    'max_fps',
    'use_wall_clock_timestamps',
    'runtime_mode',
    'print_effective_config',
    'stream_id',
    'hw_fallback_failures',
    'adaptation_enabled',
    'adaptation_profile',
    'adaptation_interval_ms',
    'adaptation_cooldown_ms',
    'pipeline_override',
}


LAUNCH_ARGUMENTS = {
    'gst_video_bridge_minimal.launch.py': COMMON_ARGUMENTS,
    'gst_video_bridge_advanced.launch.py': COMMON_ARGUMENTS | {'params_file'},
    'gst_video_bridge.launch.py': COMMON_ARGUMENTS | {'params_file'},
}


def show_args(launch_file):
    result = subprocess.run(
        ['ros2', 'launch', 'ros2_gst_video_bridge', launch_file, '--show-args'],
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    return result.stdout


def parse_argument_names(output):
    return set(re.findall(r"^    '([^']+)':", output, flags=re.MULTILINE))


def main():
    failed = False
    for launch_file, expected in LAUNCH_ARGUMENTS.items():
        output = show_args(launch_file)
        actual = parse_argument_names(output)
        missing = sorted(expected - actual)
        if missing:
            failed = True
            print(f'{launch_file}: missing launch arguments: {", ".join(missing)}')
        if 'no description given' in output:
            failed = True
            print(f'{launch_file}: every launch argument must have a description')

    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
