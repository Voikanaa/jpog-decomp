"""Download the pinned FFmpeg build that the portable jpog loads at run time for Bink videos.

Usage:
  python tools/fetch_ffmpeg.py                 download and unpack into .cache/ffmpeg/
  python tools/fetch_ffmpeg.py --install DIR   also copy the libraries next to jpog in DIR
                                               (e.g. build/x64-debug/src/shell)

The build is an LGPL shared build from BtbN/FFmpeg-Builds (GitHub), month-end release, checked by
SHA-256; it is never committed or distributed with the game. Its major versions match the vendored
headers in third_party/ffmpeg (FFmpeg 9.0). Windows x64 and Linux (x64, arm64) only: on macOS,
install FFmpeg 9 with Homebrew (`brew install ffmpeg`), where jpog finds it.
"""
import argparse
import hashlib
import os
import platform
import shutil
import sys
import tarfile
import urllib.request
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RELEASE = "autobuild-2026-09-30-13-08"
VERSION = "n9.0.2-17-g2a571b6068"
URL = f"https://github.com/BtbN/FFmpeg-Builds/releases/download/{RELEASE}/ffmpeg-{VERSION}-{{target}}-lgpl-shared-9.0.{{ext}}"
BUILDS = {
    "win64": ("zip", "7157177b8a6cb2174c1650ba8c71b363f2c78cba5330f88c4c02cf5b2b880646"),
    "linux64": ("tar.xz", "7a1ba2af36e5beeb98151e766a903b1d115f2206a86571f17f873cd7292c424f"),
    "linuxarm64": ("tar.xz", "1aa02c59afe06dea2602b40517a5cb75c0ff6d524f217b5aea1f0ca07977cd51"),
}
# The libraries jpog loads (src/shell/FFmpeg.cpp) and their major versions in FFmpeg 9.0.
LIBRARIES = {"avutil": 61, "swresample": 7, "avcodec": 63, "avformat": 63, "swscale": 10}


def target():
    machine = platform.machine().lower()
    if sys.platform == "win32" and machine in ("amd64", "x86_64"):
        return "win64"
    if sys.platform.startswith("linux") and machine in ("x86_64", "amd64"):
        return "linux64"
    if sys.platform.startswith("linux") and machine in ("aarch64", "arm64"):
        return "linuxarm64"
    if sys.platform == "darwin":
        sys.exit("macOS: install FFmpeg 9 with Homebrew (`brew install ffmpeg`); jpog finds it there")
    sys.exit(f"no pinned FFmpeg build for {sys.platform} {machine}")


def library_files(name, major):
    """(file in the build, installed name) for one library."""
    if sys.platform == "win32":
        return f"bin/{name}-{major}.dll", f"{name}-{major}.dll"
    return f"lib/lib{name}.so.{major}", f"lib{name}.so.{major}"


def download(url, path, sha256):
    if os.path.exists(path):
        with open(path, "rb") as f:
            if hashlib.sha256(f.read()).hexdigest() == sha256:
                return
    print(f"downloading {url}")
    digest = hashlib.sha256()
    partial = path + ".part"
    with urllib.request.urlopen(url) as response, open(partial, "wb") as f:
        while chunk := response.read(1 << 20):
            digest.update(chunk)
            f.write(chunk)
    if digest.hexdigest() != sha256:
        os.remove(partial)
        sys.exit(f"SHA-256 mismatch for {url}: got {digest.hexdigest()}, expected {sha256}")
    os.replace(partial, path)


def unpack(archive, ext, directory):
    """Extract only the libraries jpog loads, following symlinks to the real files."""
    wanted = {library_files(name, major)[0]: library_files(name, major)[1] for name, major in LIBRARIES.items()}
    os.makedirs(directory, exist_ok=True)
    if ext == "zip":
        with zipfile.ZipFile(archive) as z:
            for member in z.namelist():
                inner = member.split("/", 1)[-1]
                if inner in wanted:
                    with z.open(member) as src, open(os.path.join(directory, wanted[inner]), "wb") as dst:
                        shutil.copyfileobj(src, dst)
    else:
        with tarfile.open(archive) as t:
            members = {m.name.split("/", 1)[-1]: m for m in t.getmembers()}
            for inner, installed in wanted.items():
                member = members.get(inner)
                while member is not None and member.issym():
                    member = members.get(os.path.normpath(os.path.join(os.path.dirname(inner), member.linkname)))
                if member is None:
                    continue
                with t.extractfile(member) as src, open(os.path.join(directory, installed), "wb") as dst:
                    shutil.copyfileobj(src, dst)
    missing = [name for name in wanted.values() if not os.path.exists(os.path.join(directory, name))]
    if missing:
        sys.exit(f"not in the build: {', '.join(missing)}")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--install", metavar="DIR", help="copy the libraries into DIR (next to jpog)")
    args = parser.parse_args()

    name = target()
    ext, sha256 = BUILDS[name]
    cache = os.path.join(ROOT, ".cache", "ffmpeg", RELEASE)
    os.makedirs(cache, exist_ok=True)
    archive = os.path.join(cache, f"{name}.{ext}")
    download(URL.format(target=name, ext=ext), archive, sha256)
    libraries = os.path.join(cache, name)
    unpack(archive, ext, libraries)
    print(f"FFmpeg {VERSION} ({name}) in {libraries}")
    if args.install:
        os.makedirs(args.install, exist_ok=True)
        for file in sorted(os.listdir(libraries)):
            shutil.copy2(os.path.join(libraries, file), args.install)
        print(f"installed into {args.install}")


if __name__ == "__main__":
    main()
