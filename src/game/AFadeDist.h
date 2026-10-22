#pragma once

#include "common/abi.h"

// The distances at which each kind of object fades out (SimJP.exe), per draw distance level.
class AFadeDist
{
public:
	// Our names, after the Data/Constant.ini keys LoadFadeData reads; TYPE_TERRAIN and
	// TYPE_DEFAULT are never loaded (their values are the defaults below).
	enum Type
	{
		TYPE_TREE,
		TYPE_WALL,
		TYPE_DINO,
		TYPE_VEHICLE,
		TYPE_BUILDING,
		TYPE_VISITOR,
		TYPE_ROADADDON,
		TYPE_GRASS,
		TYPE_TERRAIN,
		TYPE_DEFAULT,
		TYPE_COUNT
	};

	// Takes the <level>_*FadeNearDistance / *FadeFarDistance constants: 0 low, 1 medium,
	// anything else high.
	static void TSTDCALL LoadFadeData(int level);
	static float TSTDCALL GetFadeNear(Type type);
	static float TSTDCALL GetFadeFar(Type type);

private:
	static void TSTDCALL SetFade(Type type, float nearDistance, float farDistance);

	static float sm_afFadeDistNear[TYPE_COUNT];
	static float sm_afFadeDistFar[TYPE_COUNT];
};
