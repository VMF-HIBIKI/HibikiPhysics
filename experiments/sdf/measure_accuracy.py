"""Run each plate bake in a fresh process and collect the measurements as JSON."""

import argparse
import json
import platform
import subprocess
import sys
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path, help="installed-package hibiki_sdf_accuracy executable")
    parser.add_argument("--output", type=Path, help="write JSON here (default: stdout)")
    parser.add_argument(
        "--workers", type=int, nargs="+", default=[1], help="explicit counts; the serial baseline is included"
    )
    parser.add_argument("--repeats", type=int, default=1, help="fresh measurements per configuration")
    args = parser.parse_args()
    if args.repeats < 1 or any(count < 1 for count in args.workers):
        parser.error("workers and repeats must be positive integers")
    executable = args.executable.resolve(strict=True)
    rows = []
    workers = list(dict.fromkeys([1, *args.workers]))
    for spacing in ("1", "0.5", "0.25"):
        for phase in ("0", "0.5"):
            fingerprint = None
            for repeat in range(args.repeats):
                batch = []
                order = workers if repeat % 2 == 0 else list(reversed(workers))
                for count in order:
                    print(
                        f"Baking h={spacing} mm, XY phase={phase}, workers={count}, repeat={repeat + 1}",
                        file=sys.stderr,
                        flush=True,
                    )
                    result = subprocess.run(
                        [executable, spacing, phase, str(count)], stdout=subprocess.PIPE, text=True, check=True
                    )
                    row = json.loads(result.stdout)
                    if fingerprint is None:
                        fingerprint = row["grid_fingerprint"]
                    elif row["grid_fingerprint"] != fingerprint:
                        raise ValueError(f"grid fingerprint differs for h={spacing}, phase={phase}, workers={count}")
                    row["repeat"] = repeat + 1
                    batch.append(row)
                serial_seconds = next(row["bake_s"] for row in batch if row["requested_workers"] == 1)
                for row in batch:
                    row["speedup_vs_serial"] = serial_seconds / row["bake_s"]
                rows.extend(batch)
    report = {"platform": platform.platform(), "processor": platform.processor(), "runs": rows}
    output = json.dumps(report, indent=2, allow_nan=False) + "\n"
    if args.output:
        args.output.write_text(output, encoding="utf-8", newline="\n")
    else:
        sys.stdout.write(output)


if __name__ == "__main__":
    main()
