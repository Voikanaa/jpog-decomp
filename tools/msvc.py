"""Locate MSVC command-line tools (undname, dumpbin, ...) via vswhere."""
import functools
import glob
import os
import subprocess

VSWHERE = os.path.join(os.environ.get("PROGRAMFILES(X86)", r"C:\Program Files (x86)"),
                       "Microsoft Visual Studio", "Installer", "vswhere.exe")


@functools.cache
def vs_install_dir():
    if not os.path.exists(VSWHERE):
        raise FileNotFoundError("vswhere.exe not found; install Visual Studio 2022+ or its Build Tools")
    out = subprocess.run(
        [VSWHERE, "-latest", "-products", "*",
         "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
         "-property", "installationPath"],
        capture_output=True, text=True, check=True).stdout.strip()
    if not out:
        raise FileNotFoundError("no Visual Studio install with the C++ x86/x64 build tools found")
    return out.splitlines()[0]


def tool(name):
    """Absolute path of an MSVC host-x64 tool such as 'undname.exe', newest toolset first."""
    pattern = os.path.join(vs_install_dir(), "VC", "Tools", "MSVC", "*", "bin", "Hostx64", "x64", name)
    matches = sorted(glob.glob(pattern), reverse=True)
    if not matches:
        raise FileNotFoundError(f"{name} not found under {vs_install_dir()}")
    return matches[0]
