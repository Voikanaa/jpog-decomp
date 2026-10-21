"""Configure, build and test a CMake preset.

Usage: python tools/build.py [preset] [-DOPTION=VALUE ...]

Presets (CMakePresets.json): x86-debug, x64-debug, x64-release on Windows; linux-debug,
linux-release, linux-asan on Linux; macos-debug, macos-release on macOS. The default is
x86-debug on Windows and <os>-debug elsewhere. On Windows the MSVC environment is set up from
the newest Visual Studio 2022+ install (found with vswhere), so no developer prompt is needed.
"""
import functools
import os
import platform
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def default_preset():
    system = platform.system()
    if system == "Windows":
        return "x86-debug"
    return "macos-debug" if system == "Darwin" else "linux-debug"


@functools.cache
def msvc_environment(arch):
    """The environment vcvarsall.bat sets up for `arch` (x64 or x64_x86)."""
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import msvc

    vcvars = os.path.join(msvc.vs_install_dir(), "VC", "Auxiliary", "Build", "vcvarsall.bat")
    out = subprocess.run(f'"{vcvars}" {arch} >nul && set', shell=True, capture_output=True, text=True,
                         check=True).stdout
    env = {}
    for line in out.splitlines():
        name, sep, value = line.partition("=")
        if sep:
            env[name] = value
    return env


def run(args, env):
    print("+", " ".join(args), flush=True)
    # Windows looks programs up in our own PATH, not the one passed in env.
    path = next((value for name, value in env.items() if name.upper() == "PATH"), None) if env else None
    executable = shutil.which(args[0], path=path) or args[0]
    subprocess.run([executable, *args[1:]], cwd=ROOT, env=env, check=True)


def main(argv):
    preset = argv[0] if argv and not argv[0].startswith("-") else default_preset()
    options = argv[1:] if argv and not argv[0].startswith("-") else argv
    env = None
    if platform.system() == "Windows":
        env = msvc_environment("x64_x86" if preset.startswith("x86") else "x64")
    try:
        run(["cmake", "--preset", preset, *options], env)
        run(["cmake", "--build", "--preset", preset], env)
        run(["ctest", "--test-dir", os.path.join("build", preset), "--output-on-failure"], env)
    except subprocess.CalledProcessError as error:
        return error.returncode
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
