#!/usr/bin/env python3

import argparse
import csv
import math
import time
from dataclasses import dataclass
from typing import List

import rclpy
from rclpy.node import Node
from ros2_gst_video_bridge_msgs.msg import RuntimeStatus


@dataclass
class LatencySample:
    t_wall: float
    end_to_end_ms: float
    push_ms: float


class RuntimeStatusCollector(Node):
    def __init__(self, topic: str):
        super().__init__("interlatency_benchmark_collector")
        self.samples: List[LatencySample] = []
        self.create_subscription(RuntimeStatus, topic, self._on_status, 50)

    def _on_status(self, msg: RuntimeStatus) -> None:
        self.samples.append(
            LatencySample(
                t_wall=time.time(),
                end_to_end_ms=float(msg.latency_estimate_ms),
                push_ms=float(msg.push_latency_estimate_ms),
            )
        )


def percentile(values: List[float], p: float) -> float:
    if not values:
        return float("nan")

    sorted_values = sorted(values)
    if len(sorted_values) == 1:
        return sorted_values[0]

    rank = (len(sorted_values) - 1) * (p / 100.0)
    low = int(math.floor(rank))
    high = int(math.ceil(rank))
    if low == high:
        return sorted_values[low]

    weight = rank - low
    return sorted_values[low] * (1.0 - weight) + sorted_values[high] * weight


def summarize(samples: List[LatencySample]) -> dict:
    e2e = [s.end_to_end_ms for s in samples if s.end_to_end_ms >= 0.0]
    push = [s.push_ms for s in samples if s.push_ms >= 0.0]

    return {
        "samples": len(samples),
        "e2e_p50_ms": percentile(e2e, 50.0),
        "e2e_p95_ms": percentile(e2e, 95.0),
        "e2e_p99_ms": percentile(e2e, 99.0),
        "push_p50_ms": percentile(push, 50.0),
        "push_p95_ms": percentile(push, 95.0),
        "push_p99_ms": percentile(push, 99.0),
        "push_max_ms": max(push) if push else float("nan"),
    }


def write_samples_csv(path: str, samples: List[LatencySample]) -> None:
    with open(path, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["t_wall", "latency_estimate_ms", "push_latency_estimate_ms"])
        for s in samples:
            writer.writerow([f"{s.t_wall:.6f}", f"{s.end_to_end_ms:.6f}", f"{s.push_ms:.6f}"])


def write_summary_csv(path: str, summary: dict) -> None:
    with open(path, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(summary.keys())
        writer.writerow([summary[k] for k in summary.keys()])


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Collect runtime interlatency samples and report p50/p95/p99."
    )
    parser.add_argument("--duration-sec", type=int, default=60)
    parser.add_argument("--status-topic", default="/gst_video_bridge/runtime_status")
    parser.add_argument("--samples-csv", default="/tmp/bridge_interlatency_samples.csv")
    parser.add_argument("--summary-csv", default="/tmp/bridge_interlatency_summary.csv")
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    rclpy.init()
    node = RuntimeStatusCollector(args.status_topic)

    end_ts = time.time() + max(args.duration_sec, 1)
    while time.time() < end_ts:
        rclpy.spin_once(node, timeout_sec=0.2)

    summary = summarize(node.samples)
    write_samples_csv(args.samples_csv, node.samples)
    write_summary_csv(args.summary_csv, summary)

    print("interlatency benchmark complete")
    print(f"samples: {summary['samples']}")
    print(
        "e2e latency p50/p95/p99 (ms): "
        f"{summary['e2e_p50_ms']:.3f} / {summary['e2e_p95_ms']:.3f} / {summary['e2e_p99_ms']:.3f}"
    )
    print(
        "push latency p50/p95/p99/max (ms): "
        f"{summary['push_p50_ms']:.3f} / {summary['push_p95_ms']:.3f} / "
        f"{summary['push_p99_ms']:.3f} / {summary['push_max_ms']:.3f}"
    )
    print(f"samples csv: {args.samples_csv}")
    print(f"summary csv: {args.summary_csv}")

    node.destroy_node()
    rclpy.shutdown()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
