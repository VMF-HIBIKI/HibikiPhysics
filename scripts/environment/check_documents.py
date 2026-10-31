"""Check bilingual Markdown pairs and their reciprocal language links."""

import re
import sys
from pathlib import Path


def check(path: Path) -> list[str]:
    if path.name.endswith(".zh-CN.md"):
        peer = path.with_name(path.name.removesuffix(".zh-CN.md") + ".md")
    else:
        peer = path.with_name(path.stem + ".zh-CN.md")
    if not peer.is_file():
        return [f"Bilingual document required: {peer}"]
    errors = []
    for source, destination in [(path, peer), (peer, path)]:
        text = source.read_text(encoding="utf-8")
        html_link = re.search(r"href\s*=\s*[\"']" + re.escape(destination.name) + r"[\"']", text)
        if f"]({destination.name})" not in text and not html_link:
            errors.append(f"Language switch link required: {source} -> {destination.name}")
    return errors


def main() -> int:
    errors = [error for name in sys.argv[1:] for error in check(Path(name))]
    for error in errors:
        print(error)
    return int(bool(errors))


if __name__ == "__main__":
    raise SystemExit(main())
