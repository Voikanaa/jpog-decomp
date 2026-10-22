#pragma once

#include "toshi/kernel/TObject.h"
#include "toshi/kernel/containers/TNodeTree.h"

namespace Toshi
{

class TRenderPacket;
class TTerrainInterface;
class TTerrainShader;

// A renderer resource (TRenderInterface.dll): a node of the renderer's resource tree, which is
// also the render order of shaders. Declaration only; the virtual order matches the original
// vtable (symbols/vtables.csv).
class TResource : public TObject, public TNodeTree<TResource>::TNode
{
public:
	virtual bool Create();
	virtual bool Validate();
	virtual void Invalidate();
	virtual void DestroyResource();
	virtual bool OnLostDevice();
	virtual bool OnResetDevice();

protected:
	virtual void OnDestroy();
};

// A shader resource (TRenderInterface.dll).
class TShader : public TResource
{
public:
	virtual void Flush();
	virtual void Render(TRenderPacket* packet) = 0;
};

// The terrain shader (TTerrainShaderHAL in TTerrainShaderD3D.dll). The engine defines the
// interface inline; the slots follow TTerrainShaderHAL's vtable up to the last one we use.
// UnknownN slots have no name in the original.
class TTerrainShader : public TShader
{
public:
	virtual void Unknown12() = 0;
	virtual void Unknown13() = 0;
	virtual void Unknown14() = 0;
	virtual void SetLightColours(class TVector3& ambient, class TVector3& sun) = 0;
	virtual void SetGrassModulateScale(float scale) = 0;
	virtual void SetWaterHeight(float height) = 0;
	virtual void RenderToTexture(class TTerrainMesh* mesh, unsigned int size, unsigned char* a, unsigned char* b,
	                             unsigned char* c) = 0;
	virtual void SetDropletFreq(float frequency) = 0;
	virtual void Unknown20() = 0;
	virtual void SetWindX(float x) = 0;
	virtual void SetWindY(float y) = 0;
	virtual void SetDropletMinMaxSize(float min, float max) = 0;
	virtual void SetSunTheta(float theta) = 0;
	virtual void SetSunIntensity(float intensity) = 0;
	virtual void SetBetaRayMult(float mult) = 0;
	virtual void SetBetaMieMult(float mult) = 0;
	virtual void SetInscatteringMult(float mult) = 0;
	virtual void SetHG(float hg) = 0;
	virtual void SetReflectance(float reflectance) = 0;
	virtual void SetScatteringNear(float distance) = 0;
	virtual void SetScatteringFar(float distance) = 0;
	virtual float GetDropletFreq() = 0;
	virtual void Unknown34() = 0;
	virtual float GetWindX() = 0;
	virtual float GetWindY() = 0;
	virtual float GetDropletMinSize() = 0;
	virtual float GetDropletMaxSize() = 0;
	virtual float GetSunTheta() = 0;
	virtual float GetSunIntensity() = 0;
	virtual float GetBetaRayMult() = 0;
	virtual float GetBetaMieMult() = 0;
	virtual float GetInscatteringMult() = 0;
	virtual float GetHG() = 0;
	virtual float GetReflectance() = 0;
	virtual float GetScatteringNear() = 0;
	virtual float GetScatteringFar() = 0;
	virtual class TSun* GetSun() = 0;
	virtual class TAtmosphere* GetAtmosphere() = 0;
	virtual void Unknown50() = 0;
	virtual bool IsHighEndTerrain() = 0;
	virtual bool IsTerrainDetailPass() = 0;
	virtual bool IsHighEndWater() = 0;
	virtual void EnableHighEndTerrain(bool enable) = 0;
	virtual void EnableTerrainDetailPass(bool enable) = 0;
	virtual void EnableHighEndWater(bool enable) = 0;
	virtual bool IsCapableHighEndTerrain() = 0;
	virtual bool IsCapableTerrainDetailPass() = 0;
	virtual bool IsCapableHighEndWater() = 0;
};

// The grass shader (TGrassShaderHAL in TTerrainShaderD3D.dll); slots as TGrassShaderHAL's.
class TGrassShader : public TShader
{
public:
	virtual bool IsLightScattering() = 0;
	virtual void SetLightScattering(bool enable) = 0;
	virtual bool Create(TTerrainInterface* terrain) = 0;
	virtual void ResetGrass(TTerrainInterface* terrain) = 0;
	virtual class TGrassMaterial* CreateMaterial(const char* name) = 0;
	virtual class TGrassMesh* CreateMesh(const char* name) = 0;
	virtual void SetSunTheta(float theta) = 0;
	virtual void SetSunIntensity(float intensity) = 0;
	virtual void SetBetaRayMult(float mult) = 0;
	virtual void SetBetaMieMult(float mult) = 0;
	virtual void SetInscatteringMult(float mult) = 0;
	virtual void SetHG(float hg) = 0;
	virtual void SetReflectance(float reflectance) = 0;
	virtual float GetSunTheta() = 0;
	virtual float GetSunIntensity() = 0;
	virtual float GetBetaRayMult() = 0;
	virtual float GetBetaMieMult() = 0;
	virtual float GetInscatteringMult() = 0;
	virtual float GetHG() = 0;
	virtual float GetReflectance() = 0;
	virtual void SetAmbient(class TVector3& ambient) = 0;
	virtual void SetSun(class TVector3& sun) = 0;
	virtual void SetAlphaNear(float alpha) = 0;
	virtual void SetAlphaFar(float alpha) = 0;
	virtual float GetAlphaNear() = 0;
	virtual float GetAlphaFar() = 0;
	virtual bool IsCapableGrass(TTerrainShader* terrain) = 0;
	virtual bool IsGrass() = 0;
	virtual void EnableGrass(bool enable) = 0;

	using TResource::Create;
};

// The system shader (TSysShaderHAL in TSysShaderD3D.dll); slots as TSysShaderHAL's.
class TSysShader : public TShader
{
public:
	virtual bool IsLightScattering() = 0;
	virtual void SetLightScattering(bool enable) = 0;
	virtual class TSysMaterial* CreateMaterial(const char* name) = 0;
	virtual class TSysMesh* CreateMesh(const char* name) = 0;
};

// The skin shader (TSkinShaderHAL in TSkinShaderD3D.dll); slots as TSkinShaderHAL's.
class TSkinShader : public TShader
{
public:
	virtual void SetSunTheta(float theta) = 0;
	virtual void SetSunIntensity(float intensity) = 0;
	virtual void SetBetaRayMult(float mult) = 0;
	virtual void SetBetaMieMult(float mult) = 0;
	virtual void SetInscatteringMult(float mult) = 0;
	virtual void SetHG(float hg) = 0;
	virtual void SetReflectance(float reflectance) = 0;
	virtual float GetSunTheta() = 0;
	virtual float GetSunIntensity() = 0;
	virtual float GetBetaRayMult() = 0;
	virtual float GetBetaMieMult() = 0;
	virtual float GetInscatteringMult() = 0;
	virtual float GetHG() = 0;
	virtual float GetReflectance() = 0;
	virtual void EnableRenderEnvMap(bool enable) = 0;
	virtual bool IsHighEndSkinning() = 0;
	virtual void EnableHighEndSkinning(bool enable) = 0;
	virtual bool IsCapableHighEndSkinning() = 0;
	virtual bool IsLightScattering() = 0;
	virtual void SetLightScattering(bool enable) = 0;
	virtual bool IsAlphaBlendMaterial() = 0;
	virtual void SetAlphaBlendMaterial(bool enable) = 0;
};

} // namespace Toshi
