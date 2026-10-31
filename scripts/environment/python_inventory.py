"""Validate a locked Python environment and record its installed file identities."""

import argparse
import base64
import hashlib
import importlib.metadata
import json
import sys
import tomllib
from pathlib import Path

from packaging.markers import Marker


def digest(path: Path, algorithm: str = "sha256") -> bytes:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, algorithm).digest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--warp", action="store_true")
    args = parser.parse_args()
    repository = Path(__file__).resolve().parents[2]
    environment = repository / ".venv"
    lock_path = repository / "uv.lock"
    lock = tomllib.loads(lock_path.read_text(encoding="utf-8"))
    packages = {package["name"]: package for package in lock["package"]}
    project = packages["hibiki-physics-dev"]
    groups = ["dev", "warp"] if args.warp else ["dev"]
    queue = [item["name"] for group in groups for item in project["dev-dependencies"][group]]
    expected = {}
    while queue:
        name = queue.pop()
        if name in expected:
            continue
        package = packages[name]
        if package["source"].get("registry") != "https://pypi.org/simple":
            raise ValueError(f"Expected locked PyPI source: {name}")
        expected[name] = package["version"]
        for dependency in package.get("dependencies", []):
            if not dependency.get("marker") or Marker(dependency["marker"]).evaluate():
                queue.append(dependency["name"])
    files = {}
    installed = {}
    for distribution in importlib.metadata.distributions(path=[str(environment / "Lib/site-packages")]):
        name = distribution.metadata["Name"].lower().replace("_", "-")
        installed[name] = distribution.version
        for entry in distribution.files or []:
            if str(entry).endswith((".pyc", ".pyo")):
                continue
            path = Path(str(distribution.locate_file(entry))).absolute()
            relative = Path(path).resolve().relative_to(environment.resolve()).as_posix()
            if entry.hash:
                actual = base64.urlsafe_b64encode(digest(path, entry.hash.mode)).decode().rstrip("=")
                if actual != entry.hash.value:
                    raise ValueError(f"Installed RECORD hash differs: {path}")
            files[relative] = {"sha256": digest(path).hex(), "size": path.stat().st_size}
    if installed != expected:
        raise ValueError(f"Installed package set differs: expected={expected}, actual={installed}")
    version = ".".join(map(str, sys.version_info[:3]))
    if version != (repository / ".python-version").read_text().strip():
        raise ValueError(f"Interpreter version differs: {version}")
    record = {
        "schema_version": 1,
        "lock_sha256": digest(lock_path).hex(),
        "python_version": version,
        "groups": groups,
        "packages": installed,
        "files": files,
    }
    destination = repository / ".local/python-environment.json"
    destination.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    print(f"Verified {len(installed)} locked Python packages and {len(files)} installed files")


if __name__ == "__main__":
    main()
