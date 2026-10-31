"""Check staged hooks, archive recovery and environment failure propagation."""

import hashlib
import http.server
import json
import shutil
import subprocess
import threading
from pathlib import Path

import pytest

REPOSITORY = Path(__file__).resolve().parents[2]
PREK = REPOSITORY / ".venv/Scripts/prek.exe"


def run(arguments: list[str], cwd: Path, expected: int = 0) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(arguments, cwd=cwd, text=True, encoding="utf-8", errors="replace", capture_output=True)
    assert result.returncode == expected, result.stdout + result.stderr
    return result


@pytest.fixture
def repository(tmp_path: Path) -> Path:
    run(["git", "init", "-q", "-b", "main"], tmp_path)
    run(["git", "config", "core.hooksPath", ".git/hooks"], tmp_path)
    run(["git", "config", "core.autocrlf", "false"], tmp_path)
    hook = (REPOSITORY / "scripts/environment/Hook.ps1").as_posix()
    configuration = (REPOSITORY / ".pre-commit-config.yaml").read_text(encoding="utf-8")
    configuration = configuration.replace("scripts/environment/Hook.ps1", f'"{hook}"')
    (tmp_path / ".pre-commit-config.yaml").write_bytes(configuration.encode())
    source = "value = 1\n" + "".join(f"# Context line {index}\n" for index in range(20)) + "tail = 0\n"
    (tmp_path / "sample.py").write_bytes(source.encode())
    run(["git", "add", "."], tmp_path)
    run(
        [
            "git",
            "-c",
            "user.name=Acceptance",
            "-c",
            "user.email=acceptance@example.invalid",
            "-c",
            "commit.gpgsign=false",
            "commit",
            "-qm",
            "fixture",
        ],
        tmp_path,
    )
    return tmp_path


def test_partial_staging_preserves_unstaged_work(repository: Path) -> None:
    path = repository / "sample.py"
    original = path.read_text()
    staged = original.replace("value = 1", "value=2")
    path.write_bytes(staged.encode())
    run(["git", "add", "sample.py"], repository)
    path.write_bytes(staged.replace("tail = 0", "tail = 9").encode())
    result = run([str(PREK), "run"], repository, expected=1)
    assert "Python formatting" in result.stdout + result.stderr
    assert run(["git", "show", ":sample.py"], repository).stdout == staged
    expected_worktree = staged.replace("value=2", "value = 2").replace("tail = 0", "tail = 9")
    assert path.read_text() == expected_worktree
    path.write_bytes(staged.replace("value=2", "value = 2").encode())
    run(["git", "add", "sample.py"], repository)
    path.write_bytes(expected_worktree.encode())
    run([str(PREK), "run"], repository)
    assert path.read_text() == expected_worktree
    assert "tail = 0" in run(["git", "show", ":sample.py"], repository).stdout


def test_lint_error_propagates(repository: Path) -> None:
    path = repository / "sample.py"
    path.write_bytes(b"print(missing_name)\n")
    run(["git", "add", "sample.py"], repository)
    result = run([str(PREK), "run"], repository, expected=1)
    assert "F821" in result.stdout + result.stderr
    assert path.read_bytes() == b"print(missing_name)\n"


def test_wrong_formatter_version_preserves_source(tmp_path: Path) -> None:
    script = tmp_path / "scripts/environment/Hook.ps1"
    script.parent.mkdir(parents=True)
    shutil.copyfile(REPOSITORY / "scripts/environment/Hook.ps1", script)
    (tmp_path / "tools").mkdir()
    shutil.copyfile(REPOSITORY / "tools/toolchain-lock.json", tmp_path / "tools/toolchain-lock.json")
    (tmp_path / ".local").mkdir()
    formatter = tmp_path / "fake formatter.ps1"
    formatter.write_bytes(b"Write-Output 'clang-format version 0.0.1'\r\n")
    (tmp_path / ".local/environment.json").write_bytes(json.dumps({"paths": {"clang-format": str(formatter)}}).encode())
    source = tmp_path / "sample.cpp"
    source.write_bytes(b"int value=1;\n")
    result = run(["pwsh", "-NoProfile", "-File", str(script), "-Mode", "clang-format", str(source)], tmp_path, 1)
    assert "version differs" in result.stdout
    assert source.read_bytes() == b"int value=1;\n"


PAYLOAD = b"HibikiPhysics controlled archive payload\n"


def setup(root: Path, url: str = "https://example.invalid/fixture.zip") -> tuple[Path, Path]:
    (root / "tools").mkdir()
    artifact = {
        "id": "fixture",
        "filename": "fixture.zip",
        "size": len(PAYLOAD),
        "sha256": hashlib.sha256(PAYLOAD).hexdigest(),
        "url": url,
    }
    (root / "tools/toolchain-lock.json").write_bytes(json.dumps({"artifacts": [artifact]}).encode())
    helper = root / "invoke.ps1"
    source = str(REPOSITORY / "scripts/environment/Prepare.ps1").replace("'", "''")
    helper.write_bytes(
        (
            "param([string]$Root,[string]$Import,[string]$Cache,[switch]$Offline)\r\n"
            f". '{source}'\r\n"
            "Get-HibikiArchives $Root $Import $Cache -Offline:$Offline\r\n"
        ).encode()
    )
    return helper, root / "cache" / artifact["sha256"] / "fixture.zip"


def invoke(root: Path, offline: bool = True, imports: Path | None = None) -> subprocess.CompletedProcess[str]:
    arguments = [
        "pwsh",
        "-NoProfile",
        "-File",
        str(root / "invoke.ps1"),
        "-Root",
        str(root),
        "-Cache",
        str(root / "cache"),
    ]
    if offline:
        arguments.append("-Offline")
    if imports:
        arguments += ["-Import", str(imports)]
    return subprocess.run(arguments, text=True, encoding="utf-8", errors="replace", capture_output=True, timeout=30)


def test_offline_import_and_cache_replay(tmp_path: Path) -> None:
    _, cached = setup(tmp_path)
    imports = tmp_path / "import"
    imports.mkdir()
    (imports / "fixture.zip").write_bytes(PAYLOAD)
    first = invoke(tmp_path, imports=imports)
    assert first.returncode == 0, first.stderr
    stamp = cached.stat().st_mtime_ns
    second = invoke(tmp_path)
    assert second.returncode == 0, second.stderr
    assert cached.stat().st_mtime_ns == stamp
    assert (tmp_path / ".local/archives/fixture.zip").read_bytes() == PAYLOAD


def test_offline_missing_and_bad_import(tmp_path: Path) -> None:
    _, cached = setup(tmp_path)
    missing = invoke(tmp_path)
    assert missing.returncode != 0
    assert "Offline archive missing" in missing.stderr
    imports = tmp_path / "import"
    imports.mkdir()
    (imports / "fixture.zip").write_bytes(b"incorrect")
    invalid = invoke(tmp_path, imports=imports)
    assert invalid.returncode != 0
    assert "hash mismatch" in invalid.stderr
    assert not cached.exists()


def test_resume_partial_archive(tmp_path: Path) -> None:
    ranges = []

    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            requested = self.headers.get("Range", "")
            ranges.append(requested)
            offset = int(requested.removeprefix("bytes=").removesuffix("-")) if requested else 0
            self.send_response(206 if requested else 200)
            if requested:
                self.send_header("Content-Range", f"bytes {offset}-{len(PAYLOAD) - 1}/{len(PAYLOAD)}")
            self.send_header("Content-Length", str(len(PAYLOAD) - offset))
            self.end_headers()
            self.wfile.write(PAYLOAD[offset:])

        def log_message(self, format, *args):
            pass

    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        _, cached = setup(tmp_path, f"http://127.0.0.1:{server.server_port}/fixture.zip")
        cached.parent.mkdir(parents=True)
        Path(str(cached) + ".part").write_bytes(PAYLOAD[:7])
        result = invoke(tmp_path, offline=False)
        assert result.returncode == 0, result.stderr
        assert ranges == ["bytes=7-"]
        assert cached.read_bytes() == PAYLOAD
        assert not Path(str(cached) + ".part").exists()
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)
