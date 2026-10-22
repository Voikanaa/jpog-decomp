#pragma once

#include "common/abi.h"

// The particle system (SimJP.exe). So far only the detail level and the budgets it sets; the
// task itself (created by ARootTask in the original) is not reimplemented yet (issue #21).
class AParticleSystem
{
public:
	enum PARTICLEDETAIL
	{
		PARTICLEDETAIL_LOW,
		PARTICLEDETAIL_MEDIUM,
		PARTICLEDETAIL_HIGH,
	};

	static constexpr int kMeshTypes = 20;
	static constexpr int kSystemTypes = 24;

	// Sets the vertex and mesh budgets and the fade modifier for `detail` (anything above
	// medium is high). The original also rebuilds the particle system's meshes and models when
	// it exists; ours never does yet.
	static void TSTDCALL SetDetail(PARTICLEDETAIL detail);
	static PARTICLEDETAIL GetDetail() { return m_eDetail; }
	static float GetFarDistModifier() { return m_fFarDistModifier; }
	static int GetMaxVerts() { return m_iMaxVerts; }
	static int GetMaxNumMeshes() { return m_iMaxNumMeshes; }
	static int GetNumMeshes(int type) { return m_aiNumMeshes[type]; }
	static int GetNumSystems(int type) { return m_aiNumSystems[type]; }

private:
	static PARTICLEDETAIL m_eDetail;
	static float m_fFarDistModifier;
	static int m_iMaxVerts;
	static int m_iMaxNumMeshes;
	static int m_aiNumMeshes[kMeshTypes];
	static int m_aiNumSystems[kSystemTypes];
};
