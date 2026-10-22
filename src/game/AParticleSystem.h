#pragma once

#include "common/abi.h"
#include "toshi/kernel/TScheduler.h"

// The particle system (SimJP.exe), a task under ARootTask. So far only the detail level and the
// budgets it sets; the task itself has no behaviour yet (issue #21; TClass version assumed, see
// ASystemTasks.h).
class AParticleSystem : public Toshi::TTask
{
public:
	Toshi::TClass& GetClass() const override;
	static Toshi::TClass m_sClass;

	// Any value can be passed (above medium is high), hence the fixed type.
	enum PARTICLEDETAIL : int
	{
		PARTICLEDETAIL_NONE = -1, // before the first SetDetail
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
	static Toshi::TObject* TSTDCALL CreateObject();
	static Toshi::TObject* TSTDCALL CreateObjectInPlace(void* memory);
	static void TSTDCALL InitialiseStatic();
	static void TSTDCALL DeinitialiseStatic();

	static PARTICLEDETAIL m_eDetail;
	static float m_fFarDistModifier;
	static int m_iMaxVerts;
	static int m_iMaxNumMeshes;
	static int m_aiNumMeshes[kMeshTypes];
	static int m_aiNumSystems[kSystemTypes];
};
