"""Generate the machine-specific .mcp.json from .mcp.json.example.

Usage: python tools/setup_mcp.py

.mcp.json holds absolute paths, so it is gitignored. This fills ${PROJECT_DIR} in the template
with this clone's location. Restart Claude Code afterwards so it picks up the REA server.
"""
import json
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    template = open(os.path.join(ROOT, ".mcp.json.example"), encoding="utf-8").read()
    # Escape for embedding inside a JSON string literal.
    project_dir = json.dumps(ROOT)[1:-1]
    config = template.replace("${PROJECT_DIR}", project_dir)
    json.loads(config)  # validate
    with open(os.path.join(ROOT, ".mcp.json"), "w", encoding="utf-8", newline="\n") as f:
        f.write(config)
    print(f"wrote .mcp.json for {ROOT}")


if __name__ == "__main__":
    main()
