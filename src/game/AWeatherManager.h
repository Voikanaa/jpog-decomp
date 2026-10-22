#pragma once

#include "common/abi.h"

// The park's weather (SimJP.exe). So far only the scattering offset the graphics options set.
class AWeatherManager
{
public:
	static void TSTDCALL SetScatteringNearFarOffset(float offset);
	static float GetScatteringNearFarOffset() { return sm_fScatteringNearFarOffset; }

private:
	static float sm_fScatteringNearFarOffset;
};
