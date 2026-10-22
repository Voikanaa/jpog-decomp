#include "game/ARenderer.h"

#include "game/AApplication.h"
#include "game/AOptionLogic.h"
#include "game/AOptions.h"
#include "game/ARootTask.h"

#include <new>

using namespace Toshi;

TClass ARenderer::m_sClass("ARenderer", &TTask::m_sClass, CreateObject, CreateObjectInPlace, InitialiseStatic,
                           DeinitialiseStatic, 0x10000);

TClass& ARenderer::GetClass() const
{
	return m_sClass;
}

bool ARenderer::OnUpdate(float deltaTime)
{
	const TManagedPtr<TRenderInterface> renderer = g_oTheApp.GetRootTask()->GetRenderInterface();
	if (!renderer->Update(deltaTime))
	{
		g_oTheApp.Destroy();
		return true;
	}
	renderer->BeginScene();
	renderer->EndScene();
	return true;
}

namespace
{

struct LevelOption
{
	const char* name;
	AOptionLogic::OPTION option;
};

const LevelOption kLevelOptions[] = {
    {"ScreenResolution", AOptionLogic::OPTION_SCREENRES},  {"TextureQuality", AOptionLogic::OPTION_TEXTUREQUALITY},
    {"TerrainDetail", AOptionLogic::OPTION_TERRAINDETAIL}, {"TerrainQuality", AOptionLogic::OPTION_TERRAINQUALITY},
    {"ModelQuality", AOptionLogic::OPTION_MODELQUALITY},   {"Effects", AOptionLogic::OPTION_EFFECTS},
    {"DrawDistance", AOptionLogic::OPTION_DRAWDISTANCE},
};

const LevelOption kFlagOptions[] = {
    {"Grass", AOptionLogic::OPTION_GRASS},
    {"WaterReflection", AOptionLogic::OPTION_WATERREFLECTION},
};

} // namespace

bool ARenderer::SerialiseOptions(AOptions& options)
{
	const TManagedPtr<TRenderInterface> renderer = g_oTheApp.GetRootTask()->GetRenderInterface();
	const TRenderInterface::DisplayParams* params = renderer->GetCurrentDisplayParams();
	// Saved as signed here (ARootTask::SerialiseOptions saves them unsigned).
	const int width = static_cast<int>(params->uiWidth);
	const int height = static_cast<int>(params->uiHeight);
	options.PutOption("ScreenWidth", width);
	options.PutOption("ScreenHeight", height);
	options.PutOption("ScreenWindowed", params->bWindowed);
	AOptionLogic logic;
	for (const LevelOption& level : kLevelOptions)
	{
		AOptionLogic::LMH value;
		if (logic.GetOption(level.option, value))
		{
			const int stored = value;
			options.PutOption(level.name, stored);
		}
	}
	for (const LevelOption& flag : kFlagOptions)
	{
		bool value;
		if (logic.GetOption(flag.option, value))
			options.PutOption(flag.name, value);
	}
	return true;
}

bool ARenderer::DeserialiseOptions(AOptions& options)
{
	AOptionLogic logic;
	for (const LevelOption& level : kLevelOptions)
	{
		int value;
		if (options.GetOption(level.name, value))
			logic.SetOption(level.option, static_cast<AOptionLogic::LMH>(value));
	}
	for (const LevelOption& flag : kFlagOptions)
	{
		bool value;
		if (options.GetOption(flag.name, value))
			logic.SetOption(flag.option, value);
	}
	return true;
}

TObject* TSTDCALL ARenderer::CreateObject()
{
	return new ARenderer();
}

TObject* TSTDCALL ARenderer::CreateObjectInPlace(void* memory)
{
	return memory == nullptr ? nullptr : new (memory) ARenderer();
}

void TSTDCALL ARenderer::InitialiseStatic()
{
}

void TSTDCALL ARenderer::DeinitialiseStatic()
{
}
