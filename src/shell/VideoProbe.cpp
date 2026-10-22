#include "shell/VideoProbe.h"

#include "platform/System.h"

namespace jpog::shell
{

namespace
{

// libavformat major versions: FFmpeg 4 (58) to 8 (62), newest first.
constexpr int kMajors[] = {62, 61, 60, 59, 58};

std::string LibraryName(int major)
{
	const std::string version = std::to_string(major);
#if defined(_WIN32)
	return "avformat-" + version + ".dll";
#elif defined(__APPLE__)
	return "libavformat." + version + ".dylib";
#else
	return "libavformat.so." + version;
#endif
}

} // namespace

std::string ProbeFFmpeg()
{
	for (const int major : kMajors)
	{
		const std::string name = LibraryName(major);
		void* library = platform::LibraryLoad(name.c_str());
		if (library == nullptr)
			continue;
		// unsigned avformat_version(void): major << 16 | minor << 8 | micro.
		using VersionFunction = unsigned (*)();
		auto* versionFunction = reinterpret_cast<VersionFunction>(platform::LibrarySymbol(library, "avformat_version"));
		std::string result;
		if (versionFunction != nullptr)
		{
			const unsigned version = versionFunction();
			result = name + " " + std::to_string(version >> 16) + "." + std::to_string((version >> 8) & 0xFF) + "." +
			         std::to_string(version & 0xFF);
		}
		platform::LibraryFree(library);
		if (!result.empty())
			return result;
	}
	return {};
}

} // namespace jpog::shell
