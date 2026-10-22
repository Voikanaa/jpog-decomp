#include "game/AParticleSystem.h"

// The original's initial values: no detail set yet, high budgets.
AParticleSystem::PARTICLEDETAIL AParticleSystem::m_eDetail = PARTICLEDETAIL_NONE;
float AParticleSystem::m_fFarDistModifier = 0.7f;
int AParticleSystem::m_iMaxVerts = 2000;
int AParticleSystem::m_iMaxNumMeshes = 20;
int AParticleSystem::m_aiNumMeshes[kMeshTypes];
int AParticleSystem::m_aiNumSystems[kSystemTypes];

namespace
{

// Per system type, the same at every detail level.
constexpr int kNumSystems[AParticleSystem::kSystemTypes] = {5, 5,  5, 0,  50, 1,  10, 4, 3, 0, 10, 0,
                                                            1, 25, 1, 10, 40, 15, 1,  5, 6, 0, 15, 1};

} // namespace

void TSTDCALL AParticleSystem::SetDetail(PARTICLEDETAIL detail)
{
	if (detail == m_eDetail)
		return;
	m_eDetail = detail;
	m_iMaxNumMeshes = 20;
	// The first mesh type, then types 2-10 and 16; 1 and 11-15 have none, 17-19 keep theirs.
	int first;
	int other;
	if (detail == PARTICLEDETAIL_LOW)
	{
		m_fFarDistModifier = 1.0f;
		m_iMaxVerts = 600;
		first = 8;
		other = 12;
	}
	else if (detail == PARTICLEDETAIL_MEDIUM)
	{
		m_fFarDistModifier = 0.85f;
		m_iMaxVerts = 1300;
		first = 15;
		other = 20;
	}
	else
	{
		m_fFarDistModifier = 0.7f;
		m_iMaxVerts = 2000;
		first = 20;
		other = 30;
	}
	for (int type = 0; type < kSystemTypes; ++type)
		m_aiNumSystems[type] = kNumSystems[type];
	m_aiNumMeshes[0] = first;
	m_aiNumMeshes[1] = 0;
	for (int type = 2; type <= 10; ++type)
		m_aiNumMeshes[type] = other;
	for (int type = 11; type <= 15; ++type)
		m_aiNumMeshes[type] = 0;
	m_aiNumMeshes[16] = other;
}
