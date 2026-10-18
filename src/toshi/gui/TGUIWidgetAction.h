#pragma once

#include "common/abi.h"

namespace Toshi
{

class TFreeList;

// GUI widget action (TGuiInterface.dll). Declaration only so far: the free list the game sizes at
// startup.
class TGUIWidgetAction
{
public:
	static TFreeList& TSTDCALL GetFreeList();
};

} // namespace Toshi
