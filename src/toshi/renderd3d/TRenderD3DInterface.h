#pragma once

#include "toshi/render/TRenderInterface.h"

namespace Toshi
{

// The Direct3D 8 renderer (TRenderD3DInterface.dll). Declaration only: the virtuals it adds after
// TRenderInterface's, in vtable order (symbols/vtables.csv). The game reaches them through
// TRenderInterface::GetInterface().
class TRenderD3DInterface : public TRenderInterface
{
public:
	virtual bool AdjustDisplay(const DisplayParams* params);
	// Colour correction (gamma ramp) settings.
	virtual void SetContrast(float contrast);
	virtual void SetBrightness(float brightness);
	virtual void SetGamma(float gamma);
	virtual void SetSaturate(float saturate);
	virtual float GetContrast() const;
	virtual float GetBrightness() const;
	virtual float GetGamma() const;
	virtual float GetSaturate() const;
	virtual void UpdateColourSettings();
	virtual bool IsCapableColourCorrection();
	virtual void EnableColourCorrection(bool enable);
	virtual void ForceEnableColourCorrection(bool enable);
	virtual bool IsColourCorrection();
};

} // namespace Toshi
