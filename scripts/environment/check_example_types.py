"""Check example-local type definitions with Clang and Python's syntax tree."""

import argparse
import ast
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CPP_SUFFIXES = {".cc", ".cpp", ".cxx", ".cppm", ".ixx", ".h", ".hh", ".hpp", ".hxx", ".inl", ".ipp", ".tpp"}


def locate_clang_tidy() -> str:
    config = ROOT / ".local/environment.json"
    if config.is_file():
        path = Path(json.loads(config.read_text(encoding="utf-8-sig"))["paths"]["clang-tidy"])
        if path.is_file():
            return str(path)
    command = shutil.which("clang-tidy")
    if command:
        return command
    if os.name == "nt":
        return str(Path(os.environ["ProgramFiles"]) / "LLVM/bin/clang-tidy.exe")
    return "clang-tidy"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--clang-tidy", help="Path to the locked clang-tidy executable")
    parser.add_argument("files", type=Path, nargs="*", help="Example files; defaults to all tracked examples")
    args = parser.parse_args()
    paths: list[Path] = args.files
    if not paths:
        inventory = subprocess.check_output(["git", "ls-files", "-z", "--", "examples"], cwd=ROOT)
        paths = [ROOT / name for name in inventory.decode("utf-8").split("\0") if name]

    failed = False
    for path in paths:
        if path.suffix.lower() in {".py", ".pyi"}:
            tree = ast.parse(path.read_bytes(), filename=str(path), feature_version=(3, 12))
            for node in ast.walk(tree):
                if isinstance(node, ast.ClassDef):
                    print(f"{path}:{node.lineno}: example defines class {node.name}; use an existing type")
                    failed = True

    cpp = [str(path.resolve()) for path in paths if path.suffix.lower() in CPP_SUFFIXES or path.suffix == ".C"]
    if cpp:
        manifest = json.loads((ROOT / "tools/toolchain-lock.json").read_text(encoding="utf-8"))
        version = next(item["version"] for item in manifest["artifacts"] if item["id"] == "llvm")
        clang_tidy = args.clang_tidy or locate_clang_tidy()
        identity = subprocess.check_output([clang_tidy, "--version"], text=True)
        if f"LLVM version {version}" not in (line.strip() for line in identity.splitlines()):
            raise ValueError(f"Example checks require clang-tidy {version} at {clang_tidy}")
        command = [
            clang_tidy,
            "--experimental-custom-checks",
            f"--config-file={ROOT / 'tools/quality/example-types.clang-tidy'}",
            *cpp,
            "--",
            "-x",
            "c++",
            "-std=c++20",
            f"-I{ROOT / 'include'}",
            "-Wno-pragma-once-outside-header",
        ]
        if subprocess.run(command, cwd=ROOT).returncode:
            failed = True
    return int(failed)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, SyntaxError, ValueError, subprocess.CalledProcessError) as error:
        sys.exit(str(error))
