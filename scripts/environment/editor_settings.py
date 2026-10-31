"""Render project editor templates into the local workspace and dedicated profile."""

import json
from pathlib import Path


def render(value, replacements: dict[str, str]):
    if isinstance(value, dict):
        return {key: render(child, replacements) for key, child in value.items()}
    if isinstance(value, list):
        return [render(child, replacements) for child in value]
    if isinstance(value, str):
        for token, path in replacements.items():
            value = value.replace(token, path)
    return value


def main() -> None:
    repository = Path(__file__).resolve().parents[2]
    configuration = json.loads((repository / ".local/environment.json").read_text(encoding="utf-8-sig"))
    replacements = {
        "@CLANGD@": configuration["paths"]["clangd"],
        "@PYTHON@": configuration["paths"]["python"],
        "@RUFF@": str(repository / ".venv/Scripts/ruff.exe"),
    }
    workspace = repository / ".vscode"
    workspace.mkdir(exist_ok=True)
    for name in ("settings.json", "launch.json", "tasks.json"):
        template = json.loads((repository / "tools/vscode" / name).read_text(encoding="utf-8"))
        (workspace / name).write_text(json.dumps(render(template, replacements), indent=2) + "\n", encoding="utf-8")
    profile_root = Path(configuration["editor"]["user_data"]) / "User"
    storage = json.loads((profile_root / "globalStorage/storage.json").read_text(encoding="utf-8"))
    profile = next(item for item in storage["userDataProfiles"] if item["name"] == "HibikiPhysics")
    settings_path = profile_root / "profiles" / profile["location"] / "settings.json"
    settings = json.loads(settings_path.read_text(encoding="utf-8")) if settings_path.exists() else {}
    settings.update(json.loads((workspace / "settings.json").read_text(encoding="utf-8")))
    settings.update({"extensions.autoUpdate": False, "extensions.autoCheckUpdates": False, "update.mode": "none"})
    settings_path.write_text(json.dumps(settings, indent=2) + "\n", encoding="utf-8")
    print("Project editor templates and dedicated profile settings prepared")


if __name__ == "__main__":
    main()
