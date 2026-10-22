#pragma once

#include <bgfx/bgfx.h>

struct ImDrawData;
struct ImTextureData;

namespace jpog::shell
{

// Dear ImGui renderer backend on bgfx (ImGui ships none): uploads the textures ImGui asks for
// (ImGuiBackendFlags_RendererHasTextures) and draws its command lists into one bgfx view.
class ImGuiRenderer
{
public:
	// Needs a current ImGui context and an initialised bgfx.
	bool Init(bgfx::ViewId view);
	void Shutdown();
	void Render(ImDrawData* drawData);

private:
	void UpdateTexture(ImTextureData* texture);

	bgfx::ViewId m_view = 0;
	bgfx::VertexLayout m_layout;
	bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle m_sampler = BGFX_INVALID_HANDLE;
};

} // namespace jpog::shell
