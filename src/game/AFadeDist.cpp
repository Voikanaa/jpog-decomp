#include "game/AFadeDist.h"

#include "game/AConstants.h"

using C = AConstantTypes;

// The original's initial values; LoadFadeData replaces the first eight.
float AFadeDist::sm_afFadeDistNear[TYPE_COUNT] = {140.0f, 140.0f, 140.0f, 140.0f, 140.0f,
                                                  140.0f, 140.0f, 0.0f,   290.0f, 10000.0f};
float AFadeDist::sm_afFadeDistFar[TYPE_COUNT] = {160.0f, 160.0f, 160.0f, 160.0f, 160.0f,
                                                 160.0f, 160.0f, 0.0f,   340.0f, 10001.0f};

namespace
{

struct FadeConstants
{
	AFadeDist::Type type;
	C::eFloatConstants nearDistance; // the LOW_ constant; each level's block follows
	C::eFloatConstants farDistance;
};

const FadeConstants kFades[] = {
    {AFadeDist::TYPE_TREE, C::LOW_TreeFadeNearDistance, C::LOW_TreeFadeFarDistance},
    {AFadeDist::TYPE_WALL, C::LOW_WallFadeNearDistance, C::LOW_WallFadeFarDistance},
    {AFadeDist::TYPE_DINO, C::LOW_DinoFadeNearDistance, C::LOW_DinoFadeFarDistance},
    {AFadeDist::TYPE_VEHICLE, C::LOW_VehicleFadeNearDistance, C::LOW_VehicleFadeFarDistance},
    {AFadeDist::TYPE_BUILDING, C::LOW_BuildingFadeNearDistance, C::LOW_BuildingFadeFarDistance},
    {AFadeDist::TYPE_VISITOR, C::LOW_VisitorFadeNearDistance, C::LOW_VisitorFadeFarDistance},
    {AFadeDist::TYPE_ROADADDON, C::LOW_RoadAddonFadeNearDistance, C::LOW_RoadAddonFadeFarDistance},
    {AFadeDist::TYPE_GRASS, C::LOW_GrassFadeNearDistance, C::LOW_GrassFadeFarDistance},
};

// The constants of one draw distance level, from LOW_ to MEDIUM_ to HIGH_.
constexpr int kLevelStride = C::MEDIUM_DinoFadeNearDistance - C::LOW_DinoFadeNearDistance;
static_assert(C::HIGH_GrassFadeFarDistance - C::MEDIUM_GrassFadeFarDistance == kLevelStride);
static_assert(C::MEDIUM_TreeFadeNearDistance - C::LOW_TreeFadeNearDistance == kLevelStride);

} // namespace

void TSTDCALL AFadeDist::LoadFadeData(int level)
{
	const int offset = (level == 0 ? 0 : level == 1 ? 1 : 2) * kLevelStride;
	for (const FadeConstants& fade : kFades)
		SetFade(fade.type, AConstants::GetFloatConstant(static_cast<C::eFloatConstants>(fade.nearDistance + offset)),
		        AConstants::GetFloatConstant(static_cast<C::eFloatConstants>(fade.farDistance + offset)));
}

float TSTDCALL AFadeDist::GetFadeNear(Type type)
{
	return sm_afFadeDistNear[type];
}

float TSTDCALL AFadeDist::GetFadeFar(Type type)
{
	return sm_afFadeDistFar[type];
}

void TSTDCALL AFadeDist::SetFade(Type type, float nearDistance, float farDistance)
{
	sm_afFadeDistNear[type] = nearDistance;
	sm_afFadeDistFar[type] = farDistance;
}
