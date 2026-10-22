#include "shell/BgfxCallback.h"

#include <SDL3/SDL_stdinc.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace jpog::shell
{

BgfxCallback::BgfxCallback() : m_bTrace(SDL_getenv("JPOG_BGFX_TRACE") != nullptr)
{
}

void BgfxCallback::fatal(const char* filePath, std::uint16_t line, bgfx::Fatal::Enum code, const char* message)
{
	std::fprintf(stderr, "jpog: bgfx fatal error %d at %s(%u): %s\n", static_cast<int>(code), filePath,
	             static_cast<unsigned>(line), message);
	if (code != bgfx::Fatal::DebugCheck)
		std::abort();
}

void BgfxCallback::traceVargs(const char* filePath, std::uint16_t line, const char* format, va_list arguments)
{
	if (!m_bTrace)
		return;
	std::fprintf(stderr, "%s(%u): ", filePath, static_cast<unsigned>(line));
	std::vfprintf(stderr, format, arguments);
}

void BgfxCallback::screenShot(const char* filePath, std::uint32_t width, std::uint32_t height, std::uint32_t pitch,
                              bgfx::TextureFormat::Enum format, const void* data, std::uint32_t size, bool yFlip)
{
	if (format != bgfx::TextureFormat::BGRA8 && format != bgfx::TextureFormat::RGBA8)
	{
		std::fprintf(stderr, "jpog: screenshot format %d not supported\n", static_cast<int>(format));
		return;
	}
	if (static_cast<std::size_t>(pitch) * height > size)
		return;
	// RGBA rows, top row first, opaque.
	const auto* source = static_cast<const unsigned char*>(data);
	std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 4);
	for (std::uint32_t y = 0; y < height; ++y)
	{
		const unsigned char* row = source + static_cast<std::size_t>(yFlip ? height - 1 - y : y) * pitch;
		unsigned char* out = pixels.data() + static_cast<std::size_t>(y) * width * 4;
		for (std::uint32_t x = 0; x < width; ++x, row += 4, out += 4)
		{
			const bool bgra = format == bgfx::TextureFormat::BGRA8;
			out[0] = row[bgra ? 2 : 0];
			out[1] = row[1];
			out[2] = row[bgra ? 0 : 2];
			out[3] = 0xFF;
		}
	}
	if (stbi_write_png(filePath, static_cast<int>(width), static_cast<int>(height), 4, pixels.data(),
	                   static_cast<int>(width * 4)) != 0)
		++m_iScreenshots;
	else
		std::fprintf(stderr, "jpog: could not write %s\n", filePath);
}

} // namespace jpog::shell
