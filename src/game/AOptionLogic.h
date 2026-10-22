#pragma once

#include "common/abi.h"
#include "toshi/kernel/TObject.h"

namespace Toshi
{
class TGrassShader;
class TSkinShader;
class TTerrainShader;
} // namespace Toshi

// The game's graphics settings (SimJP.exe): the screen modes the renderer offers, and the
// current values of the options menu's settings, kept in statics and applied to the renderer and
// the terrain, grass and skin shaders. Any number of instances share the statics.
class AOptionLogic : public Toshi::TObject
{
public:
	enum OPTION
	{
		OPTION_SCREENRES,
		OPTION_TEXTUREQUALITY,
		OPTION_TERRAINDETAIL,
		OPTION_TERRAINQUALITY,
		OPTION_MODELQUALITY,
		OPTION_EFFECTS,
		OPTION_DRAWDISTANCE,
		OPTION_GRASS,
		OPTION_WATERREFLECTION,
		OPTION_WINDOWED,
		OPTION_BRIGHTNESS,
		OPTION_CONTRAST,
		OPTION_GAMMA,
		OPTION_COLOURCORRECTION,
	};

	// Low, medium, high.
	enum LMH
	{
		LMH_LOW,
		LMH_MEDIUM,
		LMH_HIGH,
	};

	struct AScreenRes
	{
		unsigned int uiWidth;
		unsigned int uiHeight;
		unsigned int uiColourDepth;
		unsigned int uiUnused;
	};

	struct ASetupMode
	{
		unsigned int uiWidth;
		unsigned int uiHeight;
		unsigned int uiColourDepth;
	};

	// An option and its value; bValid tells whether the setting is available.
	struct AOptionSetting
	{
		OPTION eOption;
		bool bValid;
		union
		{
			AScreenRes oScreenRes;
			LMH eLMH;
			bool bValue;
			float fValue;
		};
	};

	AOptionLogic();
	~AOptionLogic() override;

	Toshi::TClass& GetClass() const override;

	// Collects the 4:3 hardware modes from 640x480 up, in 16 and 32 bits, from every adapter.
	void EnumerateModes();
	int GetEnumeratedModeCount() const;
	ASetupMode& GetEnumeratedMode(unsigned int index);

	// The shaders the settings apply to; creates the terrain and grass shaders if they do not
	// exist yet (the grass shader is then rendered before system resource 8).
	void GetShaders(Toshi::TTerrainShader*& terrain, Toshi::TGrassShader*& grass, Toshi::TSkinShader*& skin);
	// Fills in `setting`'s value and validity.
	void OptionGet(AOptionSetting& setting);
	// Applies `request`; `result` gets what was actually set. True when that is what was asked.
	bool AttemptOptionSet(AOptionSetting& request, AOptionSetting& result);

	bool SetOption(OPTION option, float value);
	bool SetOption(OPTION option, AScreenRes& value);
	bool SetOption(OPTION option, LMH value);
	bool SetOption(OPTION option, bool value);
	// The setting's validity.
	bool GetOption(OPTION option, float& value);
	bool GetOption(OPTION option, AScreenRes& value);
	bool GetOption(OPTION option, LMH& value);
	bool GetOption(OPTION option, bool& value);

	static Toshi::TClass m_sClass;

private:
	bool ModeExists(unsigned int width, unsigned int height, unsigned int depth);
	void AddMode(unsigned int width, unsigned int height, unsigned int depth);

	static Toshi::TObject* TSTDCALL CreateObject();
	static Toshi::TObject* TSTDCALL CreateObjectInPlace(void* memory);
	static void TSTDCALL InitialiseStatic();
	static void TSTDCALL DeinitialiseStatic();

	static AScreenRes m_oScreenRes;
	static LMH m_eTextureQuality;
	static LMH m_eTerrainDetail;
	static LMH m_eTerrainQuality;
	static LMH m_eModelQuality;
	static LMH m_eEffects;
	static LMH m_eDrawDistance;
	static bool m_bWindowed;
	static unsigned int s_ui32ModeCount;
	static ASetupMode s_aEnumeratedModeArray[];
};
