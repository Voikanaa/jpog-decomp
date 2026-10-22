#pragma once

#include <string>

namespace jpog::shell
{

struct Options
{
	// Headless check for CI: SDL's dummy video driver, bgfx's Noop renderer, miniaudio's null
	// device, and exit after `frames` frames.
	bool smoke = false;
	// 0 runs until the window is closed.
	int frames = 0;
	// bgfx renderer to use instead of the platform default: d3d11, d3d12, vulkan, opengl, metal,
	// noop.
	std::string renderer;
	// PNG file for a screenshot of the last frames (with `frames`).
	std::string screenshot;
};

// Runs the portable shell: an SDL3 window rendered with bgfx, the Dear ImGui developer menu (F1),
// miniaudio output and the kernel's scheduler. Returns the process exit code.
int Run(const Options& options);

} // namespace jpog::shell
