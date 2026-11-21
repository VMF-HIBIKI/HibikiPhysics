"""Measure the installed Release spatial benchmark, one process per repeat and configuration."""

import argparse
import hashlib
import json
import os
import platform
import subprocess
import sys
from datetime import UTC, datetime
from pathlib import Path

METHODS = ("median", "morton", "bvh4", "sap", "stackless", "batch1", "batch4", "filter", "cache")
METRICS = ("build_ms", "box_ms", "pairs_ms", "update_ms", "bake_ms", "peak_bytes")


def source_hashes() -> dict[str, str]:
    root = Path(__file__).resolve().parents[2]
    sources = (
        "tests/install-consumer/SpatialBenchmark.cpp",
        "tests/fixtures/SpatialBenchmarkScenes.h",
        "tests/fixtures/SpatialConformance.h",
        "tests/fixtures/ThroughHolePlate.h",
        "tests/fixtures/ExperimentMetrics.h",
    )
    return {name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in sources}


def processor() -> str:
    if sys.platform == "win32":
        import winreg

        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"HARDWARE\DESCRIPTION\System\CentralProcessor\0") as key:
            return str(winreg.QueryValueEx(key, "ProcessorNameString")[0]).strip()
    return platform.processor() or platform.machine()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--repeats", type=int, default=5)
    parser.add_argument("--library-revision", required=True, help="source revision of the installed library")
    args = parser.parse_args()
    if args.repeats < 1:
        parser.error("--repeats must be positive")
    executable = args.executable.resolve(strict=True)
    configurations = [(s, m) for s in ("sparse", "dense", "clustered", "thin", "moving") for m in METHODS]
    configurations += [("moving", m) for m in ("sap-update", "bvh4-update")]
    configurations += [("plate", m) for m in ("median", "morton")]
    metadata = {
        "measured_utc": datetime.now(UTC).isoformat(timespec="seconds"),
        "platform": platform.platform(),
        "processor": processor(),
        "logical_cpus": os.cpu_count(),
        "library_revision": args.library_revision,
        "executable_sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
        "benchmark_source_sha256": source_hashes(),
        "build": "Release",
        "peak_memory": "process peak working set" if sys.platform == "win32" else "process peak RSS",
        "repeats": args.repeats,
        "sample_columns": METRICS,
    }
    rows = {}
    matching = {}
    for repeat in range(args.repeats):
        # Rotate the starting case and reverse direction on alternating passes.
        first = repeat * 11 % len(configurations)
        order = configurations[first:] + configurations[:first]
        if repeat % 2:
            order.reverse()
        print(f"Release repeat {repeat + 1}/{args.repeats} ({len(order)} processes)", file=sys.stderr, flush=True)
        for scene, method in order:
            run = subprocess.run([executable, scene, method], capture_output=True, text=True)
            if run.returncode:
                raise RuntimeError(f"{scene}/{method} exited {run.returncode}: {run.stderr}")
            result = json.loads(run.stdout)
            if result.pop("build") != "Release":
                raise ValueError("measurements require a Release executable")
            compiler = result.pop("compiler")
            if compiler != metadata.setdefault("compiler", compiler):
                raise ValueError("compiler changed between runs")
            if (result["scene"], result["method"]) != (scene, method):
                raise ValueError("unexpected benchmark configuration")
            sample = [result.pop(metric, None) for metric in METRICS]
            # The child already checks the complete ID sets. Also enforce equal
            # per-frame counts (or full-grid fingerprints) across like methods.
            if scene == "plate":
                answer = (
                    result["grid_shape"],
                    result["grid_origin_m"],
                    result["spacing_mm"],
                    result["grid_fingerprint"],
                )
            else:
                answer = (result["box_hits"], result["pair_hits"])
            group = scene, method in ("filter", "cache")
            if answer != matching.setdefault(group, answer):
                raise ValueError(f"answers differ for {scene}/{method}")
            key = scene, method
            if key not in rows:
                rows[key] = (result, [])
            invariant, samples = rows[key]
            if result != invariant:
                raise ValueError(f"work counters differ across repeats for {scene}/{method}")
            samples.append(sample)
    # One JSON object per case keeps repeated samples and counter changes diffable.
    header = json.dumps(metadata, indent=2).removesuffix("\n}")
    lines = [header + ",", '  "cases": [']
    cases = [json.dumps(dict(rows[key][0], samples=rows[key][1]), allow_nan=False) for key in configurations]
    lines.extend("    " + row + ("," if i + 1 < len(cases) else "") for i, row in enumerate(cases))
    output = "\n".join([*lines, "  ]", "}", ""])
    if args.output:
        args.output.write_text(output, encoding="utf-8", newline="\n")
    else:
        sys.stdout.write(output)


if __name__ == "__main__":
    main()
