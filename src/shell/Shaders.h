#pragma once

#include <bgfx/bgfx.h>

namespace jpog::shell
{

enum class ShaderProgram
{
	ImGui,
};

// The program compiled at build time (bgfx shaderc) for the active renderer, or an invalid
// handle if this build has no shaders for it.
bgfx::ProgramHandle CreateProgram(ShaderProgram program);

} // namespace jpog::shell
