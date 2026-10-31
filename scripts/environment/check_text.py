"""Check project text endings and whitespace without modifying files."""

import sys
from pathlib import Path


def main() -> int:
    errors = []
    suffixes = {".md", ".py", ".ps1", ".cmake", ".json", ".yaml", ".yml", ".toml", ".cpp", ".h", ".hpp"}
    names = {"CMakeLists.txt", ".editorconfig", ".gitignore", ".gitattributes", ".clang-format", ".clang-tidy"}
    for name in sys.argv[1:]:
        path = Path(name)
        if path.suffix not in suffixes and path.name not in names:
            continue
        content = path.read_bytes()
        if content and not content.endswith(b"\n"):
            errors.append(f"Final newline required: {path}")
        if path.suffix == ".ps1":
            if b"\n" in content.replace(b"\r\n", b""):
                errors.append(f"PowerShell CRLF required: {path}")
        elif b"\r" in content:
            errors.append(f"LF endings required: {path}")
        for number, line in enumerate(content.decode("utf-8").splitlines(), 1):
            trailing = line[len(line.rstrip()) :]
            if trailing and not (path.suffix == ".md" and trailing == "  "):
                errors.append(f"Trailing whitespace: {path}:{number}")
    for error in errors:
        print(error)
    return int(bool(errors))


if __name__ == "__main__":
    raise SystemExit(main())
