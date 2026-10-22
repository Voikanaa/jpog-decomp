#include "game/AWeatherManager.h"

float AWeatherManager::sm_fScatteringNearFarOffset;

void TSTDCALL AWeatherManager::SetScatteringNearFarOffset(float offset)
{
	sm_fScatteringNearFarOffset = offset;
}
