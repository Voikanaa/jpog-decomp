#pragma once

#include "common/abi.h"

namespace Toshi
{
class TFreeList;
}

// Property trees (PPropertyParser.dll). Declaration only so far: the free lists the game sizes
// at startup.
class PProperties
{
public:
	class PProperty
	{
	public:
		static Toshi::TFreeList& TSTDCALL GetFreeList();
	};

	static Toshi::TFreeList& TSTDCALL GetFreeList();
};
