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
    args = parser.parse_args()
    executable = args.executable.resolve(strict=True)
    rows = []
    for spacing in ("1", "0.5", "0.25"):
        for phase in ("0", "0.5"):
            print(f"Baking h={spacing} mm, XY phase={phase}", file=sys.stderr, flush=True)
            result = subprocess.run([executable, spacing, phase], stdout=subprocess.PIPE, text=True, check=True)
            rows.append(json.loads(result.stdout))
    report = {"platform": platform.platform(), "processor": platform.processor(), "runs": rows}
    output = json.dumps(report, indent=2, allow_nan=False) + "\n"
    if args.output:
        args.output.write_text(output, encoding="utf-8", newline="\n")
    else:
        sys.stdout.write(output)


if __name__ == "__main__":
    main()
