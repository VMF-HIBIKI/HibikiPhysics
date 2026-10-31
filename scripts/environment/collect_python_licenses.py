"""Record transitive Python notices from the verified locked installation."""

import hashlib
import importlib.metadata
import json
import re
import tomllib
from pathlib import Path
from urllib.parse import unquote, urlparse

from packaging.utils import parse_wheel_filename


def main() -> None:
    root = Path(__file__).resolve().parents[2]
    lock_path = root / "uv.lock"
    lock = tomllib.loads(lock_path.read_text(encoding="utf-8"))
    manifest = json.loads((root / "tools/toolchain-lock.json").read_text(encoding="utf-8"))
    direct = {item["id"].removesuffix("-wheel") for item in manifest["artifacts"] if item["kind"] == "python_wheel"}
    packages = {item["name"]: item for item in lock["package"]}
    inventory = json.loads((root / ".local/python-environment.json").read_text(encoding="utf-8"))
    lock_hash = hashlib.sha256(lock_path.read_bytes()).hexdigest()
    if inventory["lock_sha256"] != lock_hash:
        raise ValueError("Installed Python inventory must match the lock")
    records = []
    for name in sorted(inventory["packages"].keys() - direct):
        distribution = importlib.metadata.distribution(name)
        tags = set(re.findall(r"(?m)^Tag: (.+)$", distribution.read_text("WHEEL") or ""))
        candidates = []
        for wheel in packages[name]["wheels"]:
            filename = unquote(urlparse(wheel["url"]).path.rsplit("/", 1)[-1])
            if {str(tag) for tag in parse_wheel_filename(filename)[3]} == tags:
                candidates.append(wheel)
        if len(candidates) != 1:
            raise ValueError(f"Unique locked wheel identity required: {name}")
        wheel = candidates[0]
        for member in distribution.files or []:
            if not (
                ".dist-info/licenses/" in str(member)
                or re.match(r"(?i)(licen[sc]e|copying|notice|copyright)", member.name)
            ):
                continue
            source = Path(str(distribution.locate_file(member)))
            relative = source.resolve().relative_to((root / ".venv").resolve()).as_posix()
            data = source.read_bytes()
            digest = hashlib.sha256(data).hexdigest()
            if inventory["files"][relative]["sha256"] != digest:
                raise ValueError(f"Notice differs from verified installation: {source}")
            filename = digest + ".txt"
            destination = root / "third_party/licenses/python-transitive" / name / filename
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
            records.append(
                {
                    "package": name,
                    "version": distribution.version,
                    "wheel_url": wheel["url"],
                    "wheel_sha256": wheel["hash"].removeprefix("sha256:"),
                    "member": str(member),
                    "path": destination.relative_to(root).as_posix(),
                    "sha256": digest,
                }
            )
    output = {"lock_sha256": lock_hash, "notices": records}
    (root / "third_party/licenses/python-lock-sources.json").write_bytes((json.dumps(output, indent=2) + "\n").encode())
    print(f"Recorded {len(records)} transitive Python license texts")


if __name__ == "__main__":
    main()
