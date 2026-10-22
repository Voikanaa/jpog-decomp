#include "shell/FFmpeg.h"

#include "platform/System.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/ffversion.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <iterator>

// The function's type comes from the vendored headers; nothing links against FFmpeg.
#define JPOG_FFMPEG_SYMBOL(library, function) \
	reinterpret_cast<decltype(&function)>(platform::LibrarySymbol(library, #function))

namespace jpog::shell
{

namespace
{

struct Library
{
	const char* name;
	int major;
};

// Dependencies first, so each library finds the ones it needs already loaded.
constexpr Library kLibraries[] = {
    {"avutil", LIBAVUTIL_VERSION_MAJOR},   {"swresample", LIBSWRESAMPLE_VERSION_MAJOR},
    {"avcodec", LIBAVCODEC_VERSION_MAJOR}, {"avformat", LIBAVFORMAT_VERSION_MAJOR},
    {"swscale", LIBSWSCALE_VERSION_MAJOR},
};
enum : int
{
	kAvUtil,
	kSwResample,
	kAvCodec,
	kAvFormat,
	kSwScale,
};

std::string FileName(const Library& library)
{
	const std::string major = std::to_string(library.major);
#if defined(_WIN32)
	return library.name + ("-" + major) + ".dll";
#elif defined(__APPLE__)
	return "lib" + (library.name + ("." + major)) + ".dylib";
#else
	return "lib" + (library.name + (".so." + major));
#endif
}

} // namespace

FFmpeg::~FFmpeg()
{
	Unload();
}

bool FFmpeg::Load(const std::string& baseDirectory)
{
	Unload();
	// jpog's directory, the system's library paths, and on macOS Homebrew's prefixes, which
	// dlopen does not search.
	const std::string directories[] = {
	    baseDirectory,
	    "",
#if defined(__APPLE__)
	    "/opt/homebrew/lib/",
	    "/usr/local/lib/",
#endif
	};
	for (const std::string& directory : directories)
		if (LoadFrom(directory))
			return true;
	m_sDescription = "not found (needs " + FileName(kLibraries[kAvFormat]) + " and the other libraries of FFmpeg " +
	                 FFMPEG_VERSION + " or a release with the same major versions; videos are skipped)";
	return false;
}

bool FFmpeg::LoadFrom(const std::string& directory)
{
	for (std::size_t i = 0; i < std::size(kLibraries); ++i)
	{
		m_aLibraries[i] = platform::LibraryLoad((directory + FileName(kLibraries[i])).c_str());
		if (m_aLibraries[i] == nullptr)
		{
			Unload();
			return false;
		}
	}

	// unsigned <library>_version(void): major << 16 | minor << 8 | micro.
	const decltype(&avutil_version) versions[] = {
	    JPOG_FFMPEG_SYMBOL(m_aLibraries[kAvUtil], avutil_version),
	    JPOG_FFMPEG_SYMBOL(m_aLibraries[kSwResample], swresample_version),
	    JPOG_FFMPEG_SYMBOL(m_aLibraries[kAvCodec], avcodec_version),
	    JPOG_FFMPEG_SYMBOL(m_aLibraries[kAvFormat], avformat_version),
	    JPOG_FFMPEG_SYMBOL(m_aLibraries[kSwScale], swscale_version),
	};
	auto* versionInfo = JPOG_FFMPEG_SYMBOL(m_aLibraries[kAvUtil], av_version_info);
	auto* findDecoder = JPOG_FFMPEG_SYMBOL(m_aLibraries[kAvCodec], avcodec_find_decoder);
	bool matches = versionInfo != nullptr && findDecoder != nullptr;
	for (std::size_t i = 0; matches && i < std::size(kLibraries); ++i)
		matches = versions[i] != nullptr && static_cast<int>(versions[i]() >> 16) == kLibraries[i].major;
	if (!matches)
	{
		Unload();
		return false;
	}

	const bool bink = findDecoder(AV_CODEC_ID_BINKVIDEO) != nullptr &&
	                  findDecoder(AV_CODEC_ID_BINKAUDIO_DCT) != nullptr &&
	                  findDecoder(AV_CODEC_ID_BINKAUDIO_RDFT) != nullptr;
	m_sDescription = std::string(versionInfo()) + (directory.empty() ? " (system)" : " (" + directory + ")") +
	                 (bink ? ", Bink video and audio" : ", no Bink decoders");
	return true;
}

void FFmpeg::Unload()
{
	// Dependents first.
	for (std::size_t i = std::size(m_aLibraries); i-- > 0;)
	{
		if (m_aLibraries[i] != nullptr)
			platform::LibraryFree(m_aLibraries[i]);
		m_aLibraries[i] = nullptr;
	}
}

} // namespace jpog::shell
