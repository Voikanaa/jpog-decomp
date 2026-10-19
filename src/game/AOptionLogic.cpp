#include "game/AOptionLogic.h"

#include "toshi/render/TRenderInterface.h"
#include "toshi/render/TResource.h"
#include "toshi/renderd3d/TRenderD3DInterface.h"

#include <new>

using namespace Toshi;

TClass AOptionLogic::m_sClass("AOptionLogic", &TObject::m_sClass, CreateObject, CreateObjectInPlace, InitialiseStatic,
                              DeinitialiseStatic, 0x10001);

AOptionLogic::AScreenRes AOptionLogic::m_oScreenRes = {640, 480, 32, 0};
AOptionLogic::LMH AOptionLogic::m_eTextureQuality = LMH_HIGH;
AOptionLogic::LMH AOptionLogic::m_eTerrainDetail = LMH_HIGH;
AOptionLogic::LMH AOptionLogic::m_eTerrainQuality = LMH_HIGH;
AOptionLogic::LMH AOptionLogic::m_eModelQuality = LMH_MEDIUM;
AOptionLogic::LMH AOptionLogic::m_eEffects = LMH_MEDIUM;
AOptionLogic::LMH AOptionLogic::m_eDrawDistance = LMH_MEDIUM;
bool AOptionLogic::m_bWindowed = false;
unsigned int AOptionLogic::s_ui32ModeCount = 0;

namespace
{

// Deviation: the original's array has no bound check (it ends in the zero-filled data).
constexpr unsigned int kMaxModes = 64;

TRenderD3DInterface* Renderer()
{
	return static_cast<TRenderD3DInterface*>(TRenderInterface::GetInterface());
}

} // namespace

AOptionLogic::ASetupMode AOptionLogic::s_aEnumeratedModeArray[kMaxModes];

AOptionLogic::AOptionLogic()
{
}

AOptionLogic::~AOptionLogic()
{
}

TClass& AOptionLogic::GetClass() const
{
	return m_sClass;
}

void AOptionLogic::EnumerateModes()
{
	TNodeList<TRenderAdapter>* adapters = TRenderInterface::GetInterface()->GetAdapterList();
	for (TRenderAdapter* adapter = adapters->Begin(); adapter != adapters->End(); adapter = adapter->Next())
	{
		TNodeList<TRenderAdapter::Mode>* modes = adapter->GetModeList();
		for (TRenderAdapter::Mode* mode = modes->Begin(); mode != modes->End(); mode = mode->Next())
		{
			for (int i = 0; i < adapter->GetNumSupportedDevices(); ++i)
			{
				const TRenderAdapter::Mode::Device* device = mode->GetDevice(i);
				if (device->IsSoftware())
					continue;
				// The original also queries these without using the results.
				if (!device->GetMode()->Is32Bit())
					device->GetMode()->Is16Bit();
				const unsigned int width = device->GetMode()->GetWidth();
				const unsigned int height = device->GetMode()->GetHeight();
				device->GetMode()->Unknown7();
				if (width < 640 || height < 480 || width != height / 3 * 4)
					continue;
				if (device->GetMode()->Is16Bit() && !ModeExists(width, height, 16))
					AddMode(width, height, 16);
				if (device->GetMode()->Is32Bit() && !ModeExists(width, height, 32))
					AddMode(width, height, 32);
			}
		}
	}
}

int AOptionLogic::GetEnumeratedModeCount() const
{
	return static_cast<int>(s_ui32ModeCount);
}

AOptionLogic::ASetupMode& AOptionLogic::GetEnumeratedMode(unsigned int index)
{
	return s_aEnumeratedModeArray[index];
}

bool AOptionLogic::ModeExists(unsigned int width, unsigned int height, unsigned int depth)
{
	for (unsigned int i = 0; i < s_ui32ModeCount; ++i)
	{
		const ASetupMode& mode = s_aEnumeratedModeArray[i];
		if (mode.uiWidth == width && mode.uiHeight == height && mode.uiColourDepth == depth)
			return true;
	}
	return false;
}

void AOptionLogic::AddMode(unsigned int width, unsigned int height, unsigned int depth)
{
	if (s_ui32ModeCount >= kMaxModes)
		return;
	s_aEnumeratedModeArray[s_ui32ModeCount++] = {width, height, depth};
}

void AOptionLogic::GetShaders(TTerrainShader*& terrain, TGrassShader*& grass, TSkinShader*& skin)
{
	TRenderInterface* renderer = TRenderInterface::GetInterface();
	terrain = static_cast<TTerrainShader*>(renderer->FindOrphanedResource("SHTERRAIN", nullptr));
	grass = static_cast<TGrassShader*>(renderer->FindOrphanedResource("SHGRASS", nullptr));
	skin = static_cast<TSkinShader*>(renderer->GetSystemResource(static_cast<TRenderInterface::SYSRESOURCES>(9)));
	if (terrain == nullptr)
	{
		TResource* parent = renderer->GetSystemResource(static_cast<TRenderInterface::SYSRESOURCES>(7));
		terrain = static_cast<TTerrainShader*>(
		    renderer->CreateResource(TClass::Find("TTerrainShaderHAL", nullptr), "SHTERRAIN", parent));
		terrain->Create();
	}
	if (grass != nullptr)
		return;

	TResource* parent = renderer->GetSystemResource(static_cast<TRenderInterface::SYSRESOURCES>(7));
	grass = static_cast<TGrassShader*>(
	    renderer->CreateResource(TClass::Find("TGrassShaderHAL", nullptr), "SHGRASS", parent));
	grass->Create(nullptr);

	// Moves system resource 8 to the end of its siblings, so it comes after the new shader. The
	// original reads the parent's children without a check, so a top-level resource 8 would
	// crash it; here it moves to the end of the top level.
	TResource* moved = renderer->GetSystemResource(static_cast<TRenderInterface::SYSRESOURCES>(8));
	TNodeTree<TResource>* tree = renderer->GetResourceTree();
	TResource* siblingsParent = moved->Parent();
	if (siblingsParent == tree->Root())
		siblingsParent = nullptr;
	tree->Remove(moved, false);
	tree->InsertTail(siblingsParent != nullptr ? siblingsParent : tree->Root(), moved);
}

void AOptionLogic::OptionGet(AOptionSetting& setting)
{
	TTerrainShader* terrain;
	TGrassShader* grass;
	TSkinShader* skin;
	GetShaders(terrain, grass, skin);
	TRenderD3DInterface* renderer = Renderer();
	switch (setting.eOption)
	{
	case OPTION_SCREENRES:
		setting.oScreenRes = m_oScreenRes;
		setting.bValid = true;
		break;
	case OPTION_TEXTUREQUALITY:
		setting.eLMH = m_eTextureQuality;
		setting.bValid = true;
		break;
	case OPTION_TERRAINDETAIL:
		setting.eLMH = m_eTerrainDetail;
		setting.bValid = true;
		break;
	case OPTION_TERRAINQUALITY:
	{
		// What the terrain shader can do caps the stored quality.
		const LMH capability = terrain->IsHighEndTerrain()      ? LMH_HIGH
		                       : terrain->IsTerrainDetailPass() ? LMH_MEDIUM
		                                                        : LMH_LOW;
		if (capability < m_eTerrainQuality)
			m_eTerrainQuality = capability;
		setting.eLMH = m_eTerrainQuality;
		setting.bValid = true;
		break;
	}
	case OPTION_MODELQUALITY:
		setting.eLMH = m_eModelQuality;
		setting.bValid = true;
		break;
	case OPTION_EFFECTS:
		setting.eLMH = m_eEffects;
		setting.bValid = true;
		break;
	case OPTION_DRAWDISTANCE:
		setting.eLMH = m_eDrawDistance;
		setting.bValid = true;
		break;
	case OPTION_GRASS:
		setting.bValue = grass->IsGrass();
		setting.bValid = grass->IsCapableGrass(terrain);
		break;
	case OPTION_WATERREFLECTION:
		setting.bValue = terrain->IsHighEndWater();
		setting.bValid = terrain->IsCapableHighEndWater();
		if (setting.bValid)
			setting.bValid = terrain->IsHighEndTerrain();
		break;
	case OPTION_WINDOWED:
		setting.bValue = m_bWindowed;
		setting.bValid = false;
		// Quirk: falls through, as in the original, so the brightness overwrites the value.
		[[fallthrough]];
	case OPTION_BRIGHTNESS:
		setting.bValid = renderer->IsColourCorrection();
		setting.fValue = renderer->GetBrightness();
		break;
	case OPTION_CONTRAST:
		setting.bValid = renderer->IsColourCorrection();
		setting.fValue = renderer->GetContrast();
		break;
	case OPTION_GAMMA:
		setting.bValid = renderer->IsColourCorrection();
		setting.fValue = renderer->GetGamma();
		break;
	case OPTION_COLOURCORRECTION:
		setting.bValid = renderer->IsCapableColourCorrection();
		setting.bValue = renderer->IsColourCorrection();
		break;
	}
}

bool AOptionLogic::AttemptOptionSet(AOptionSetting& request, AOptionSetting& result)
{
	TTerrainShader* terrain;
	TGrassShader* grass;
	TSkinShader* skin;
	GetShaders(terrain, grass, skin);
	TRenderD3DInterface* renderer = Renderer();
	result.eOption = request.eOption;
	switch (request.eOption)
	{
	case OPTION_SCREENRES:
		result = request;
		m_oScreenRes = request.oScreenRes;
		return true;
	case OPTION_TEXTUREQUALITY:
		result = request;
		m_eTextureQuality = request.eLMH;
		return true;
	case OPTION_TERRAINDETAIL:
		result = request;
		m_eTerrainDetail = request.eLMH;
		return true;
	case OPTION_TERRAINQUALITY:
	{
		if (request.eLMH == LMH_LOW || request.eLMH == LMH_MEDIUM)
		{
			terrain->EnableHighEndTerrain(false);
			terrain->EnableTerrainDetailPass(request.eLMH == LMH_MEDIUM);
			terrain->EnableHighEndWater(false);
			skin->SetAlphaBlendMaterial(false);
		}
		else if (request.eLMH == LMH_HIGH)
		{
			terrain->EnableHighEndTerrain(true);
			skin->SetAlphaBlendMaterial(true);
		}
		// What the shader accepted.
		const bool highEnd = terrain->IsHighEndTerrain();
		const bool detailPass = terrain->IsTerrainDetailPass();
		const LMH quality = highEnd ? LMH_HIGH : detailPass ? LMH_MEDIUM : LMH_LOW;
		result.eLMH = quality;
		m_eTerrainQuality = quality;
		return quality == request.eLMH;
	}
	case OPTION_MODELQUALITY:
		result = request;
		m_eModelQuality = request.eLMH;
		return true;
	case OPTION_EFFECTS:
		result = request;
		m_eEffects = request.eLMH;
		return true;
	case OPTION_DRAWDISTANCE:
		result = request;
		m_eDrawDistance = request.eLMH;
		return true;
	case OPTION_GRASS:
		grass->EnableGrass(request.bValue);
		result.bValue = grass->IsGrass();
		return result.bValue == request.bValue;
	case OPTION_WATERREFLECTION:
		terrain->EnableHighEndWater(request.bValue);
		result.bValue = terrain->IsHighEndWater();
		return result.bValue == request.bValue;
	case OPTION_WINDOWED:
		result = request;
		m_bWindowed = request.bValue;
		return true;
	case OPTION_BRIGHTNESS:
		renderer->SetBrightness(request.fValue);
		result.fValue = renderer->GetBrightness();
		return result.fValue == request.fValue;
	case OPTION_CONTRAST:
		renderer->SetContrast(request.fValue);
		result.fValue = renderer->GetContrast();
		return result.fValue == request.fValue;
	case OPTION_GAMMA:
		renderer->SetGamma(request.fValue);
		result.fValue = renderer->GetGamma();
		return result.fValue == request.fValue;
	case OPTION_COLOURCORRECTION:
		renderer->EnableColourCorrection(request.bValue);
		result.bValue = renderer->IsColourCorrection();
		return result.bValue == request.bValue;
	}
	return false;
}

bool AOptionLogic::SetOption(OPTION option, float value)
{
	AOptionSetting request{};
	AOptionSetting result{};
	request.eOption = option;
	request.fValue = value;
	return AttemptOptionSet(request, result);
}

bool AOptionLogic::SetOption(OPTION option, AScreenRes& value)
{
	AOptionSetting request{};
	AOptionSetting result{};
	request.eOption = option;
	request.oScreenRes = value;
	return AttemptOptionSet(request, result);
}

bool AOptionLogic::SetOption(OPTION option, LMH value)
{
	AOptionSetting request{};
	AOptionSetting result{};
	request.eOption = option;
	request.eLMH = value;
	return AttemptOptionSet(request, result);
}

bool AOptionLogic::SetOption(OPTION option, bool value)
{
	AOptionSetting request{};
	AOptionSetting result{};
	request.eOption = option;
	request.bValue = value;
	return AttemptOptionSet(request, result);
}

bool AOptionLogic::GetOption(OPTION option, float& value)
{
	AOptionSetting setting{};
	setting.eOption = option;
	OptionGet(setting);
	value = setting.fValue;
	return setting.bValid;
}

bool AOptionLogic::GetOption(OPTION option, AScreenRes& value)
{
	AOptionSetting setting{};
	setting.eOption = option;
	OptionGet(setting);
	value = setting.oScreenRes;
	return setting.bValid;
}

bool AOptionLogic::GetOption(OPTION option, LMH& value)
{
	AOptionSetting setting{};
	setting.eOption = option;
	OptionGet(setting);
	value = setting.eLMH;
	return setting.bValid;
}

bool AOptionLogic::GetOption(OPTION option, bool& value)
{
	AOptionSetting setting{};
	setting.eOption = option;
	OptionGet(setting);
	value = setting.bValue;
	return setting.bValid;
}

TObject* __stdcall AOptionLogic::CreateObject()
{
	return new AOptionLogic();
}

TObject* __stdcall AOptionLogic::CreateObjectInPlace(void* memory)
{
	return memory == nullptr ? nullptr : new (memory) AOptionLogic();
}

void __stdcall AOptionLogic::InitialiseStatic()
{
}

void __stdcall AOptionLogic::DeinitialiseStatic()
{
}
