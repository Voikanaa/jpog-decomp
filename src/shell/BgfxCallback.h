#pragma once

#include <bgfx/bgfx.h>
#include <cstdarg>
#include <cstdint>

namespace jpog::shell
{

// bgfx callbacks: screenshots as PNG (stb_image_write), fatal errors to stderr. bgfx's debug
// trace is printed only when the environment variable JPOG_BGFX_TRACE is set.
class BgfxCallback : public bgfx::CallbackI
{
public:
	BgfxCallback();

	void fatal(const char* filePath, std::uint16_t line, bgfx::Fatal::Enum code, const char* message) override;
	void traceVargs(const char* filePath, std::uint16_t line, const char* format, va_list arguments) override;
	void profilerBegin(const char*, std::uint32_t, const char*, std::uint16_t) override {}
	void profilerBeginLiteral(const char*, std::uint32_t, const char*, std::uint16_t) override {}
	void profilerEnd() override {}
	std::uint32_t cacheReadSize(std::uint64_t) override { return 0; }
	bool cacheRead(std::uint64_t, void*, std::uint32_t) override { return false; }
	void cacheWrite(std::uint64_t, const void*, std::uint32_t) override {}
	void screenShot(const char* filePath, std::uint32_t width, std::uint32_t height, std::uint32_t pitch,
	                bgfx::TextureFormat::Enum format, const void* data, std::uint32_t size, bool yFlip) override;
	void captureBegin(std::uint32_t, std::uint32_t, std::uint32_t, bgfx::TextureFormat::Enum, bool) override {}
	void captureEnd() override {}
	void captureFrame(const void*, std::uint32_t) override {}

	// Screenshots written so far.
	int Screenshots() const { return m_iScreenshots; }

private:
	bool m_bTrace;
	int m_iScreenshots = 0;
};

} // namespace jpog::shell
