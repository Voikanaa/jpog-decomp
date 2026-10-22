#include "shell/ImGuiRenderer.h"

#include "shell/Shaders.h"

#include <bx/math.h>
#include <cstdint>
#include <cstring>
#include <imgui.h>

namespace jpog::shell
{

namespace
{

// ImTextureID 0 means "no texture" (ImTextureID_Invalid), but 0 is a valid bgfx handle index.
ImTextureID ToTextureId(bgfx::TextureHandle handle)
{
	return static_cast<ImTextureID>(handle.idx) + 1;
}

bgfx::TextureHandle FromTextureId(ImTextureID id)
{
	return {static_cast<std::uint16_t>(id - 1)};
}

// Copies a rectangle of an RGBA32 ImGui texture into bgfx memory.
const bgfx::Memory* CopyRect(ImTextureData* texture, int x, int y, int width, int height)
{
	const int pitch = width * texture->BytesPerPixel;
	const bgfx::Memory* memory = bgfx::alloc(static_cast<std::uint32_t>(pitch * height));
	for (int row = 0; row < height; ++row)
		std::memcpy(memory->data + row * pitch, texture->GetPixelsAt(x, y + row), static_cast<std::size_t>(pitch));
	return memory;
}

} // namespace

bool ImGuiRenderer::Init(bgfx::ViewId view)
{
	m_view = view;
	ImGuiIO& io = ImGui::GetIO();
	io.BackendRendererName = "jpog_bgfx";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;

	m_layout.begin()
	    .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();
	m_sampler = bgfx::createUniform("s_texture", bgfx::UniformType::Sampler);
	// The Noop renderer (headless smoke runs) has no shader format: textures are still managed,
	// nothing is drawn.
	if (bgfx::getRendererType() == bgfx::RendererType::Noop)
		return true;
	m_program = CreateProgram(ShaderProgram::ImGui);
	return bgfx::isValid(m_program);
}

void ImGuiRenderer::Shutdown()
{
	for (ImTextureData* texture : ImGui::GetPlatformIO().Textures)
		if (texture->RefCount == 1 && texture->GetTexID() != ImTextureID_Invalid)
		{
			bgfx::destroy(FromTextureId(texture->GetTexID()));
			texture->SetTexID(ImTextureID_Invalid);
			texture->SetStatus(ImTextureStatus_Destroyed);
		}
	if (bgfx::isValid(m_program))
		bgfx::destroy(m_program);
	if (bgfx::isValid(m_sampler))
		bgfx::destroy(m_sampler);
	m_program = BGFX_INVALID_HANDLE;
	m_sampler = BGFX_INVALID_HANDLE;
	ImGuiIO& io = ImGui::GetIO();
	io.BackendRendererName = nullptr;
	io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures);
}

void ImGuiRenderer::UpdateTexture(ImTextureData* texture)
{
	IM_ASSERT(texture->Format == ImTextureFormat_RGBA32);
	switch (texture->Status)
	{
	case ImTextureStatus_WantCreate:
	{
		const bgfx::TextureHandle handle = bgfx::createTexture2D(static_cast<std::uint16_t>(texture->Width),
		                                                         static_cast<std::uint16_t>(texture->Height), false, 1,
		                                                         bgfx::TextureFormat::RGBA8, 0, nullptr);
		bgfx::updateTexture2D(handle, 0, 0, 0, 0, static_cast<std::uint16_t>(texture->Width),
		                      static_cast<std::uint16_t>(texture->Height),
		                      CopyRect(texture, 0, 0, texture->Width, texture->Height));
		texture->SetTexID(ToTextureId(handle));
		texture->SetStatus(ImTextureStatus_OK);
		break;
	}
	case ImTextureStatus_WantUpdates:
	{
		const bgfx::TextureHandle handle = FromTextureId(texture->GetTexID());
		for (const ImTextureRect& rect : texture->Updates)
			bgfx::updateTexture2D(handle, 0, 0, rect.x, rect.y, rect.w, rect.h,
			                      CopyRect(texture, rect.x, rect.y, rect.w, rect.h));
		texture->SetStatus(ImTextureStatus_OK);
		break;
	}
	case ImTextureStatus_WantDestroy:
		if (texture->GetTexID() != ImTextureID_Invalid)
			bgfx::destroy(FromTextureId(texture->GetTexID()));
		texture->SetTexID(ImTextureID_Invalid);
		texture->SetStatus(ImTextureStatus_Destroyed);
		break;
	default:
		break;
	}
}

void ImGuiRenderer::Render(ImDrawData* drawData)
{
	if (drawData->Textures != nullptr)
		for (ImTextureData* texture : *drawData->Textures)
			if (texture->Status != ImTextureStatus_OK)
				UpdateTexture(texture);

	const float width = drawData->DisplaySize.x * drawData->FramebufferScale.x;
	const float height = drawData->DisplaySize.y * drawData->FramebufferScale.y;
	if (width <= 0.0f || height <= 0.0f || !bgfx::isValid(m_program))
		return;

	bgfx::setViewName(m_view, "ImGui");
	bgfx::setViewMode(m_view, bgfx::ViewMode::Sequential);
	bgfx::setViewRect(m_view, 0, 0, static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height));
	const ImVec2 origin = drawData->DisplayPos;
	const ImVec2 size = drawData->DisplaySize;
	float projection[16];
	bx::mtxOrtho(projection, origin.x, origin.x + size.x, origin.y + size.y, origin.y, 0.0f, 1000.0f, 0.0f,
	             bgfx::getCaps()->homogeneousDepth);
	bgfx::setViewTransform(m_view, nullptr, projection);

	const ImVec2 scale = drawData->FramebufferScale;
	const std::uint64_t state =
	    BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA |
	    BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA, BGFX_STATE_BLEND_ONE,
	                                   BGFX_STATE_BLEND_INV_SRC_ALPHA);
	for (const ImDrawList* list : drawData->CmdLists)
	{
		const auto vertexCount = static_cast<std::uint32_t>(list->VtxBuffer.Size);
		const auto indexCount = static_cast<std::uint32_t>(list->IdxBuffer.Size);
		static_assert(sizeof(ImDrawIdx) == 2, "bgfx transient index buffers are 16-bit");
		if (bgfx::getAvailTransientVertexBuffer(vertexCount, m_layout) < vertexCount ||
		    bgfx::getAvailTransientIndexBuffer(indexCount) < indexCount)
			break; // out of transient memory this frame: drop the rest
		bgfx::TransientVertexBuffer vertices;
		bgfx::TransientIndexBuffer indices;
		bgfx::allocTransientVertexBuffer(&vertices, vertexCount, m_layout);
		bgfx::allocTransientIndexBuffer(&indices, indexCount);
		std::memcpy(vertices.data, list->VtxBuffer.Data, vertexCount * sizeof(ImDrawVert));
		std::memcpy(indices.data, list->IdxBuffer.Data, indexCount * sizeof(ImDrawIdx));

		for (const ImDrawCmd& command : list->CmdBuffer)
		{
			if (command.UserCallback != nullptr)
			{
				if (command.UserCallback != ImDrawCallback_ResetRenderState)
					command.UserCallback(list, &command);
				continue;
			}
			const float left = (command.ClipRect.x - origin.x) * scale.x;
			const float top = (command.ClipRect.y - origin.y) * scale.y;
			const float right = (command.ClipRect.z - origin.x) * scale.x;
			const float bottom = (command.ClipRect.w - origin.y) * scale.y;
			if (right <= left || bottom <= top || command.ElemCount == 0)
				continue;
			bgfx::setScissor(static_cast<std::uint16_t>(left > 0.0f ? left : 0.0f),
			                 static_cast<std::uint16_t>(top > 0.0f ? top : 0.0f),
			                 static_cast<std::uint16_t>(right - (left > 0.0f ? left : 0.0f)),
			                 static_cast<std::uint16_t>(bottom - (top > 0.0f ? top : 0.0f)));
			bgfx::setState(state);
			bgfx::setTexture(0, m_sampler, FromTextureId(command.GetTexID()));
			bgfx::setVertexBuffer(0, &vertices, command.VtxOffset, vertexCount - command.VtxOffset);
			bgfx::setIndexBuffer(&indices, command.IdxOffset, command.ElemCount);
			bgfx::submit(m_view, m_program);
		}
	}
}

} // namespace jpog::shell
