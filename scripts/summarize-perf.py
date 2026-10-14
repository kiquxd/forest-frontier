#!/usr/bin/env python3
"""Read compact JSONL logs; output only aggregates and the worst windows."""
import argparse
import json
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("logs", nargs="*", type=Path)
args = parser.parse_args()
paths = args.logs or sorted(Path(__file__).resolve().parent.parent.joinpath("logs").glob("perf-*.jsonl"))[-1:]
if not paths:
    parser.error("No performance logs found")
for path in paths:
    events = [json.loads(line) for line in path.read_text().splitlines() if line.strip()]
    windows = [e for e in events if e.get("event") == "perf"]
    llm = [e for e in events if e.get("event") == "llm_result"]
    start = next((e for e in events if e.get("event") == "session_start"), {})
    actual = windows[-1] if windows else start
    framebuffer = f", framebuffer={actual['framebuffer_width']}x{actual['framebuffer_height']}" if "framebuffer_width" in actual else ""
    print(f"\n{path.name}: actual={actual.get('width')}x{actual.get('height')}{framebuffer}, requested={start.get('width')}x{start.get('height')}, limit={start.get('fps_limit')}, LLM={start.get('benchmark_llm')}")
    if not windows:
        print("No frame windows")
        continue
    count = sum(e["frames"] for e in windows)
    duration = sum(e["frame_ms"]["mean"] * e["frames"] for e in windows)
    print(f"Frames={count}, weighted FPS={count * 1000 / duration:.1f}, game RSS max={max(e.get('game_rss_mb', 0) for e in windows):.1f} MiB")
    print(f"Frames >20 ms={sum(e['frames_over_20ms'] for e in windows)}, LLM busy frames={sum(e['llm_busy_frames'] for e in windows)}")
    last = windows[-1]
    if "attacks" in last:
        print(f"Gameplay: attacks={last['attacks']}, kills={last['kills']}, damage={last['damage_taken']}, respawns={last['respawns']}, biome={last['biome']}")
        print(f"Navigation: plans={last['path_plans']}, total={last['path_ms_total']:.2f} ms, max={last['path_ms_max']:.2f} ms")
    if "known_surface_cells" in last:
        print(f"Exploration: surface={last['known_surface_cells']}, underwater={last['known_underwater_cells']}; technologies={last['technologies']}, walls={last['walls']}, owned weapons={last['weapons']}")
        stages = [e for e in events if e.get("event", "").startswith("benchmark_")]
        failed = [e["event"] for e in stages if not e.get("ok", True)]
        print(f"Benchmark stages={len(stages)}, failures={failed or 'none'}")
    for label, key in [("idle", "llm_idle_p95_ms"), ("LLM busy", "llm_busy_p95_ms")]:
        values = [e[key] for e in windows if e.get(key) is not None]
        if values:
            print(f"Worst window p95 ({label})={max(values):.3f} ms")
    print("Worst windows (these are window percentiles, not a global percentile):")
    for e in sorted(windows, key=lambda e: e["frame_ms"]["p99"], reverse=True)[:3]:
        print(f"  t={e['elapsed_s']:.1f}s fps={e['fps']:.1f} p95={e['frame_ms']['p95']:.3f} p99={e['frame_ms']['p99']:.3f} max={e['frame_ms']['max']:.3f} ms; update={e['update_ms_mean']:.3f}, submit={e['render_submit_ms_mean']:.3f}, present={e['present_wait_ms_mean']:.3f}")
    ok = [e for e in llm if e["ok"]]
    print(f"LLM requests completed={len(llm)}, OK={len(ok)}, failed={len(llm) - len(ok)}")
    if ok:
        print(f"LLM mean first token={sum(e['first_token_ms'] for e in ok) / len(ok):.0f} ms, mean total={sum(e['total_ms'] for e in ok) / len(ok):.0f} ms")
    for e in llm:
        if not e["ok"]:
            print(f"  LLM failure: {e['error']}")
