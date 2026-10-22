#pragma once

#include <string>

namespace jpog::shell
{

// The game's Bink videos will be decoded by FFmpeg, loaded at run time (docs/plan.md, Video):
// from the system on Linux and macOS, from a pinned shared build next to the game on Windows.
// Without it, videos are skipped. This finds libavformat and reports its version, or an empty
// string.
std::string ProbeFFmpeg();

} // namespace jpog::shell
