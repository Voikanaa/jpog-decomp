#pragma once

#include "common/abi.h"
#include "common/layout.h"
#include "toshi/kernel/TObject.h"
#include "toshi/kernel/TRefCounted.h"
#include "toshi/kernel/containers/TNodeList.h"
#include "toshi/kernel/containers/TNodeTree.h"
#include "toshi/render/TRenderAdapter.h"

namespace Toshi
{

class TKernelInterface;
class TMaterialLibraryManager;
class TMatrix44;
class TModel;
class TRenderCapture;
class TRenderContext;
class TResource;
class TVector2;
class TViewport;

// Abstract renderer (TRenderInterface.dll); TRenderD3DInterface.dll implements it.
// Declaration only so far. The virtual order matches the original vtable (symbols/vtables.csv);
// instances are always created by the engine (TClass::CreateObject), never by us. Holders count references in the
// TRefCounted part (offset 4) and delete the renderer when the count drops to zero.
class TRenderInterface : public TObject, public TRefCounted
{
public:
	class DisplayParams
	{
	public:
		unsigned int uiWidth;
		unsigned int uiHeight;
		unsigned int uiColourDepth;
		unsigned int eDepthStencilFormat;
		bool bWindowed;
	};

	enum PRIMITIVETYPE : int
	{
	};

	// The renderer's built-in resources; the game uses 7 (shader parent), 8 and 9 (skin shader).
	// The names of the values are not exported.
	enum SYSRESOURCES : int
	{
	};

	virtual bool CreateDisplay(const DisplayParams* params) = 0;
	virtual bool DestroyDisplay() = 0;
	// Pumps window messages; returns false once the window has been closed.
	virtual bool Update(float deltaTime) = 0;
	virtual bool BeginScene() = 0;
	virtual bool EndScene() = 0;
	virtual void* GetCurrentDevice() = 0;
	virtual const DisplayParams* GetCurrentDisplayParams() = 0;
	virtual bool Create(TKernelInterface* kernel);
	virtual bool Destroy();
	virtual void RenderIndexPrimitive(PRIMITIVETYPE type, unsigned int a, unsigned int b, const unsigned short* indices,
	                                  const void* vertices, unsigned short stride);
	virtual void DumpStats();
	virtual void GetScreenOffset(TVector2& offset);
	virtual void SetScreenOffset(TVector2& offset);
	virtual float GetScreenAspectRatio();
	virtual float GetPixelAspectRatio();
	virtual bool SetPixelAspectRatio(float ratio);
	virtual bool IsTextureFormatSupported(int format);
	virtual bool Supports32BitTextures();
	virtual TRenderContext* CreateRenderContext() = 0;
	virtual TRenderCapture* CreateCapture() = 0;
	virtual void DestroyCapture(TRenderCapture* capture) = 0;
	virtual void SetLightDirectionMatrix(const TMatrix44& matrix);
	virtual void SetLightColourMatrix(const TMatrix44& matrix);
	virtual void ConnectDefaultViewportHandlers(TViewport& viewport);
	virtual TModel* CreateModel(const char* name, bool load) = 0;
	virtual void FailedD3DMessageBox();
	virtual void FailedDisplayMessageBox();

	TNodeList<TRenderAdapter>* GetAdapterList();
	// The first device that can show the mode `params` asks for, or null.
	const TRenderAdapter::Mode::Device* FindDevice(const DisplayParams* params);
	// Looks a resource up by name (the game passes a null parent).
	TResource* FindOrphanedResource(const char* name, TResource* parent);
	TResource* GetSystemResource(SYSRESOURCES resource);
	TResource* CreateResource(const TClass* type, const char* name, TResource* parent);
	TNodeTree<TResource>* GetResourceTree();
	// The original game reads the member (+0x128) inline.
	TMaterialLibraryManager* GetMaterialLibraryManager();

	// The renderer; the original reads the private static inline.
	static TRenderInterface* GetInterface() { return s_Interface; }

protected:
	virtual bool CreateSystemResources();
	virtual void DestroySystemResources();

private:
	TIMPORT_DATA static TRenderInterface* s_Interface;
};

JPOG_ASSERT_SIZE(TRenderInterface::DisplayParams, 0x14);

} // namespace Toshi
