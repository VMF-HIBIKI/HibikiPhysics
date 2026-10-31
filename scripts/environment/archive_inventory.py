"""Compare installed payloads with a pinned archive and record verified file identities."""

import argparse
import hashlib
import json
import tarfile
import zipfile
from pathlib import Path, PurePosixPath


def file_hash(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def canonical(path: Path) -> Path:
    resolved = str(path.resolve())
    if resolved.startswith("\\\\?\\UNC\\"):
        resolved = "\\\\" + resolved[8:]
    elif resolved.startswith("\\\\?\\"):
        resolved = resolved[4:]
    return Path(resolved)


def verify(archive: Path, root: Path, artifact: dict) -> dict:
    if archive.stat().st_size != artifact["size"] or file_hash(archive) != artifact["sha256"]:
        raise ValueError(f"Archive identity differs: {artifact['id']}")
    strip = 1 if artifact["id"] in {"llvm", "cmake", "python"} else 0
    extension = artifact["kind"] == "vscode_extension"
    records: dict[str, dict] = {}

    def record(member: str, data: bytes) -> None:
        parts = PurePosixPath(member).parts
        if extension:
            if not parts or parts[0] != "extension":
                return
            parts = parts[1:]
        else:
            parts = parts[strip:]
        if not parts or ".." in parts or PurePosixPath(*parts).is_absolute():
            raise ValueError(f"Invalid payload path: {member}")
        relative = PurePosixPath(*parts).as_posix()
        if artifact["id"] == "python" and ("__pycache__" in parts or relative.endswith((".pyc", ".pyo"))):
            return
        target = root.joinpath(*parts)
        if not canonical(target).is_relative_to(canonical(root)):
            raise ValueError(f"Payload escapes its root: {member}")
        expected = hashlib.sha256(data).hexdigest()
        actual = file_hash(target)
        if extension and relative == "package.json":
            upstream = json.loads(data)
            installed = json.loads(target.read_text(encoding="utf-8"))
            upstream.pop("__metadata", None)
            installed.pop("__metadata", None)
            if upstream != installed:
                raise ValueError(f"Extension manifest differs: {target}")
        elif actual != expected:
            raise ValueError(f"Installed payload differs: {target}")
        records[relative] = {"sha256": actual, "size": target.stat().st_size}

    if zipfile.is_zipfile(archive):
        with zipfile.ZipFile(archive) as source:
            for entry in source.infolist():
                if not entry.is_dir():
                    record(entry.filename, source.read(entry))
    else:
        links = []
        with tarfile.open(archive, "r|*") as source:
            for entry in source:
                if entry.isfile():
                    stream = source.extractfile(entry)
                    if stream is None:
                        raise ValueError(f"Payload stream required: {entry.name}")
                    record(entry.name, stream.read())
                elif entry.islnk():
                    links.append(entry)
                elif entry.issym():
                    raise ValueError(f"Explicit symbolic-link validation required: {entry.name}")
        for link in links:
            target_name = PurePosixPath(*PurePosixPath(link.linkname).parts[strip:]).as_posix()
            relative = PurePosixPath(*PurePosixPath(link.name).parts[strip:]).as_posix()
            expected = records[target_name]
            target = root / relative
            if not canonical(target).is_relative_to(canonical(root)) or file_hash(target) != expected["sha256"]:
                raise ValueError(f"Hardlink payload differs: {relative}")
            records[relative] = dict(expected)
    if not records:
        raise ValueError("A verified payload inventory must contain files")
    return {
        "schema_version": 1,
        "artifact": artifact["id"],
        "version": artifact["version"],
        "archive_sha256": artifact["sha256"],
        "root": str(root.absolute()),
        "files": records,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifact", required=True)
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    repository = Path(__file__).resolve().parents[2]
    manifest = json.loads((repository / "tools/toolchain-lock.json").read_text(encoding="utf-8"))
    artifact = next(item for item in manifest["artifacts"] if item["id"] == args.artifact)
    receipt = verify(args.archive, args.root, artifact)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(f"Verified {artifact['id']}: {len(receipt['files'])} payload files")


if __name__ == "__main__":
    main()
