#pragma once

#include "common/abi.h"

namespace Toshi
{

// The terrain engine (TTerrainInterface.dll). Declaration only: so far the level of detail.
class TTerrainInterface
{
public:
	// Our names; the original only has the type.
	enum TERRAINLOD
	{
		TERRAINLOD_LOW,
		TERRAINLOD_MEDIUM,
		TERRAINLOD_HIGH,
	};

	static TERRAINLOD TSTDCALL GetLODDetail();
	static void TSTDCALL SetLODDetail(TERRAINLOD lod);
};

} // namespace Toshi
