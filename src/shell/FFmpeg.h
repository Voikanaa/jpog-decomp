#pragma once

#include <string>

namespace jpog::shell
{

// The game's Bink videos will be decoded by FFmpeg (libavcodec decodes Bink 1 video and audio),
// loaded at run time so jpog neither links nor ships it (docs/plan.md, Video). The vendored
// headers (FFmpeg 9.0, third_party/ffmpeg) fix the ABI, so only libraries of the same major
// versions are used: first next to jpog (where tools/fetch_ffmpeg.py installs a pinned build),
// then from the system. Without them, videos are skipped.
class FFmpeg
{
public:
	FFmpeg() = default;
	FFmpeg(const FFmpeg&) = delete;
	FFmpeg& operator=(const FFmpeg&) = delete;
	~FFmpeg();

	// `baseDirectory` is jpog's own directory (with a trailing separator). False if no matching
	// set of libraries loads.
	bool Load(const std::string& baseDirectory);
	void Unload();

	bool IsLoaded() const { return m_aLibraries[0] != nullptr; }
	// Version, location and Bink support, or which libraries were looked for.
	const std::string& Describe() const { return m_sDescription; }

private:
	bool LoadFrom(const std::string& directory);

	void* m_aLibraries[5] = {};
	std::string m_sDescription;
};

} // namespace jpog::shell
