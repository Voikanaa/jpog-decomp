#include "shell/Shell.h"

#include <SDL3/SDL_main.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{

void PrintUsage()
{
	std::printf(
	    "Usage: jpog [--renderer d3d11|d3d12|vulkan|opengl|metal|noop] [--frames N] [--screenshot FILE] [--smoke [N]]\n"
	    "  --smoke [N]   headless check: dummy video, Noop renderer, null audio, N frames (60)\n");
}

} // namespace

int main(int argc, char* argv[])
{
	jpog::shell::Options options;
	for (int i = 1; i < argc; ++i)
	{
		const char* argument = argv[i];
		const bool hasValue = i + 1 < argc && argv[i + 1][0] != '-';
		if (std::strcmp(argument, "--smoke") == 0)
		{
			options.smoke = true;
			options.frames = hasValue ? std::atoi(argv[++i]) : 60;
		}
		else if (std::strcmp(argument, "--frames") == 0 && hasValue)
			options.frames = std::atoi(argv[++i]);
		else if (std::strcmp(argument, "--renderer") == 0 && hasValue)
			options.renderer = argv[++i];
		else if (std::strcmp(argument, "--screenshot") == 0 && hasValue)
			options.screenshot = argv[++i];
		else
		{
			PrintUsage();
			return std::strcmp(argument, "--help") == 0 ? 0 : 2;
		}
	}
	return jpog::shell::Run(options);
}
