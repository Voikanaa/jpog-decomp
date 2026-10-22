#include "shell/Shell.h"

#include "shell/Audio.h"
#include "shell/BgfxCallback.h"
#include "shell/ImGuiRenderer.h"
#include "shell/VideoProbe.h"
#include "toshi/kernel/TKernelInterface.h"
#include "toshi/kernel/TScheduler.h"

#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <cstdint>
#include <cstdio>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <memory>
#include <tracy/Tracy.hpp>

namespace jpog::shell
{

namespace
{

constexpr bgfx::ViewId kSceneView = 0;
constexpr bgfx::ViewId kImGuiView = 255;

struct RendererName
{
	const char* name;
	bgfx::RendererType::Enum type;
};

constexpr RendererName kRenderers[] = {
    {"d3d11", bgfx::RendererType::Direct3D11}, {"d3d12", bgfx::RendererType::Direct3D12},
    {"vulkan", bgfx::RendererType::Vulkan},    {"opengl", bgfx::RendererType::OpenGL},
    {"metal", bgfx::RendererType::Metal},      {"noop", bgfx::RendererType::Noop},
};

bool ParseRenderer(const std::string& name, bgfx::RendererType::Enum& type)
{
	for (const RendererName& renderer : kRenderers)
		if (name == renderer.name)
		{
			type = renderer.type;
			return true;
		}
	return false;
}

// The platform default: Direct3D 11 on Windows (as bgfx picks it), Metal on macOS, and bgfx's
// choice (Vulkan, else OpenGL) on Linux.
bgfx::RendererType::Enum DefaultRenderer()
{
#if defined(_WIN32)
	return bgfx::RendererType::Direct3D11;
#elif defined(__APPLE__)
	return bgfx::RendererType::Metal;
#else
	return bgfx::RendererType::Count;
#endif
}

// The native window for bgfx's main swap chain. On macOS a Metal view's layer, which `metalView`
// keeps alive.
bool GetNativeWindow(SDL_Window* window, bgfx::Init& init, SDL_MetalView& metalView)
{
	bgfx::SwapChain& data = init.swapChain;
	const SDL_PropertiesID properties = SDL_GetWindowProperties(window);
#if defined(_WIN32)
	(void)metalView;
	data.nwh = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif defined(__APPLE__)
	(void)properties;
	metalView = SDL_Metal_CreateView(window);
	data.nwh = metalView != nullptr ? SDL_Metal_GetLayer(metalView) : nullptr;
#else
	(void)metalView;
	const char* driver = SDL_GetCurrentVideoDriver();
	if (driver != nullptr && SDL_strcmp(driver, "wayland") == 0)
	{
		data.ndt = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
		data.nwh = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
		init.platformData.type = bgfx::NativeWindowHandleType::Wayland;
	}
	else
	{
		data.ndt = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
		// bgfx takes the X11 window ID (an integer) in its pointer field.
		const auto id =
		    static_cast<std::uintptr_t>(SDL_GetNumberProperty(properties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
		data.nwh = reinterpret_cast<void*>(id);
	}
#endif
	return data.nwh != nullptr;
}

struct DeveloperMenu
{
	bool visible = true;
	bool showDemo = false;
	std::string audio;
	std::string ffmpeg;
};

void DrawDeveloperMenu(DeveloperMenu& menu, Toshi::TKernelInterface& kernel, int width, int height)
{
	ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Developer (F1)", &menu.visible, ImGuiWindowFlags_AlwaysAutoResize))
	{
		const ImGuiIO& io = ImGui::GetIO();
		ImGui::TextUnformatted("jpog-decomp portable shell");
		ImGui::Separator();
		ImGui::Text("Renderer: %s", bgfx::getRendererName(bgfx::getRendererType()));
		ImGui::Text("Resolution: %d x %d", width, height);
		ImGui::Text("Frame: %.2f ms (%.0f FPS)", io.Framerate > 0.0f ? 1000.0f / io.Framerate : 0.0f,
		            static_cast<double>(io.Framerate));
		ImGui::Text("Kernel frames: %d", kernel.GetScheduler()->GetFrameCount());
		ImGui::Text("Audio: %s", menu.audio.c_str());
		ImGui::Text("FFmpeg: %s", menu.ffmpeg.empty() ? "not found (videos are skipped)" : menu.ffmpeg.c_str());
		ImGui::Checkbox("Dear ImGui demo", &menu.showDemo);
	}
	ImGui::End();
	if (menu.showDemo)
		ImGui::ShowDemoWindow(&menu.showDemo);
}

} // namespace

int Run(const Options& options)
{
	bgfx::RendererType::Enum rendererType = options.smoke ? bgfx::RendererType::Noop : DefaultRenderer();
	if (!options.renderer.empty() && !ParseRenderer(options.renderer, rendererType))
	{
		std::fprintf(stderr, "jpog: unknown renderer '%s'\n", options.renderer.c_str());
		return 2;
	}
	const bool headless = rendererType == bgfx::RendererType::Noop;

	if (options.smoke)
		SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
	{
		std::fprintf(stderr, "jpog: SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}

	SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#if defined(__APPLE__)
	if (!headless)
		windowFlags |= SDL_WINDOW_METAL;
#endif
	SDL_Window* window = SDL_CreateWindow("Jurassic Park: Operation Genesis (jpog-decomp)", 1024, 768, windowFlags);
	if (window == nullptr)
	{
		std::fprintf(stderr, "jpog: SDL_CreateWindow failed: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}
	int width = 0;
	int height = 0;
	SDL_GetWindowSizeInPixels(window, &width, &height);

	// Render on this thread (bgfx's single-threaded mode).
	bgfx::renderFrame();
	bgfx::Init init;
	init.type = rendererType;
	init.swapChain.width = static_cast<std::uint32_t>(width);
	init.swapChain.height = static_cast<std::uint32_t>(height);
	init.reset = options.smoke ? BGFX_RESET_NONE : BGFX_RESET_VSYNC;
	BgfxCallback callback;
	init.callback = &callback;
	SDL_MetalView metalView = nullptr;
	if (!headless && !GetNativeWindow(window, init, metalView))
	{
		std::fprintf(stderr, "jpog: no native window handle for video driver '%s'\n", SDL_GetCurrentVideoDriver());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}
	if (!bgfx::init(init))
	{
		std::fprintf(stderr, "jpog: bgfx could not start a renderer\n");
		if (metalView != nullptr)
			SDL_Metal_DestroyView(metalView);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}
	bgfx::setViewClear(kSceneView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x1e2a1eff, 1.0f, 0);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO().IniFilename = nullptr; // nothing written next to the game until user paths exist
	ImGui::StyleColorsDark();
	ImGui_ImplSDL3_InitForOther(window);
	ImGuiRenderer imguiRenderer;
	if (!imguiRenderer.Init(kImGuiView))
		std::fprintf(stderr, "jpog: no ImGui shaders for %s; the developer menu is not drawn\n",
		             bgfx::getRendererName(bgfx::getRendererType()));

	auto kernel = std::make_unique<Toshi::TKernelInterface>(0, nullptr, false);

	DeveloperMenu menu;
	Audio audio;
	if (!audio.Init(options.smoke))
		std::fprintf(stderr, "jpog: no audio device; continuing without sound\n");
	menu.audio = audio.Describe();
	menu.ffmpeg = ProbeFFmpeg();
	std::printf("jpog: renderer %s, %d x %d, audio: %s, FFmpeg: %s\n", bgfx::getRendererName(bgfx::getRendererType()),
	            width, height, menu.audio.c_str(), menu.ffmpeg.empty() ? "not found" : menu.ffmpeg.c_str());

	int frame = 0;
	bool running = true;
	while (running)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL3_ProcessEvent(&event);
			if (event.type == SDL_EVENT_QUIT)
				running = false;
			else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
			{
				width = event.window.data1;
				height = event.window.data2;
				bgfx::SwapChain swapChain;
				swapChain.width = static_cast<std::uint32_t>(width);
				swapChain.height = static_cast<std::uint32_t>(height);
				// Headless runs have no main window to resize.
				bgfx::reset(init.reset, headless ? nullptr : &swapChain);
			}
			else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F1 && !event.key.repeat)
				menu.visible = !menu.visible;
		}

		kernel->Update();

		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
		if (menu.visible)
			DrawDeveloperMenu(menu, *kernel, width, height);
		ImGui::Render();

		bgfx::setViewRect(kSceneView, 0, 0, static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height));
		bgfx::touch(kSceneView);
		imguiRenderer.Render(ImGui::GetDrawData());
		bgfx::frame();
		FrameMark;

		++frame;
		// Requested a few frames before the end: the image arrives once the frame is rendered.
		if (!options.screenshot.empty() && frame == (options.frames > 3 ? options.frames - 3 : 1))
			bgfx::requestScreenShot(BGFX_INVALID_HANDLE, options.screenshot.c_str());
		if (options.frames > 0 && frame >= options.frames)
			running = false;
	}

	audio.Shutdown();
	kernel.reset();
	imguiRenderer.Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
	bgfx::shutdown();
	if (metalView != nullptr)
		SDL_Metal_DestroyView(metalView);
	SDL_DestroyWindow(window);
	SDL_Quit();
	const bool screenshotMissing = !options.screenshot.empty() && callback.Screenshots() == 0;
	if (!options.screenshot.empty())
		std::printf("jpog: %s %s\n", screenshotMissing ? "could not write" : "wrote", options.screenshot.c_str());
	if (options.smoke)
		std::printf("jpog: smoke run finished after %d frames\n", frame);
	return screenshotMissing ? 1 : 0;
}

} // namespace jpog::shell
