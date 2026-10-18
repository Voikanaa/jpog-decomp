#pragma once

#include "common/abi.h"

namespace Toshi
{

class TFreeList;

// Direct3D 8 resources (TRenderD3DInterface.dll). Declarations only so far: the free lists the
// game sizes at startup.
class TTextureResourceHAL
{
public:
	static TFreeList& TSTDCALL GetFreeList();
};

class TIndexBlockResource
{
public:
	static TFreeList& TSTDCALL GetFreeList();
};

class TVertexBlockResource
{
public:
	static TFreeList& TSTDCALL GetFreeList();
};

} // namespace Toshi
