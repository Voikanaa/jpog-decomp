"""Generate the machine-specific .mcp.json from .mcp.json.example.

Usage: python tools/setup_mcp.py [--ghidra GHIDRA_DIR]

.mcp.json holds absolute paths, so it is gitignored. This fills ${PROJECT_DIR} in the template
with this clone's location and ${GHIDRA_DIR} with the Ghidra install (--ghidra, else
$GHIDRA_INSTALL_DIR, else the value already in .mcp.json). tools/ghidra_export.py reads the same
setting. Restart Claude Code afterwards so it picks up the REA server.
"""
import argparse
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MCP_JSON = os.path.join(ROOT, ".mcp.json")


def configured_ghidra():
    """GHIDRA_INSTALL_DIR from an existing .mcp.json, or None."""
    try:
        with open(MCP_JSON, encoding="utf-8") as f:
            return json.load(f)["mcpServers"]["rea"]["env"]["GHIDRA_INSTALL_DIR"]
    except (OSError, ValueError, KeyError, TypeError):
        return None


def is_ghidra(path):
    return bool(path) and os.path.exists(os.path.join(path, "support", "analyzeHeadless.bat"))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--ghidra", help="Ghidra install directory")
    args = parser.parse_args()

    ghidra = args.ghidra or os.environ.get("GHIDRA_INSTALL_DIR") or configured_ghidra()
    if not is_ghidra(ghidra):
        sys.exit(f"Ghidra not found at {ghidra!r}: pass --ghidra <dir containing support/analyzeHeadless.bat>")
    ghidra = os.path.abspath(ghidra)

    template = open(os.path.join(ROOT, ".mcp.json.example"), encoding="utf-8").read()
    # Escape for embedding inside a JSON string literal.
    config = template.replace("${PROJECT_DIR}", json.dumps(ROOT)[1:-1])
    config = config.replace("${GHIDRA_DIR}", json.dumps(ghidra)[1:-1])
    json.loads(config)  # validate
    with open(MCP_JSON, "w", encoding="utf-8", newline="\n") as f:
        f.write(config)
    print(f"wrote .mcp.json for {ROOT} (Ghidra: {ghidra})")


if __name__ == "__main__":
    main()
