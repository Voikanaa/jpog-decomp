#include "game/ASystemTasks.h"

#include <new>

using namespace Toshi;

#define JPOG_PLACEHOLDER_TASK(Name) \
	TClass Name::m_sClass(#Name, &TTask::m_sClass, CreateObject, CreateObjectInPlace, InitialiseStatic, \
	                      DeinitialiseStatic, 0x10000); \
	TClass& Name::GetClass() const \
	{ \
		return m_sClass; \
	} \
	TObject* TSTDCALL Name::CreateObject() \
	{ \
		return new Name(); \
	} \
	TObject* TSTDCALL Name::CreateObjectInPlace(void* memory) \
	{ \
		return memory == nullptr ? nullptr : new (memory) Name(); \
	} \
	void TSTDCALL Name::InitialiseStatic() \
	{ \
	} \
	void TSTDCALL Name::DeinitialiseStatic() \
	{ \
	}

JPOG_PLACEHOLDER_TASK(AToshiAnimationSystem)
JPOG_PLACEHOLDER_TASK(ACameraManager)
JPOG_PLACEHOLDER_TASK(AGUISystem)
JPOG_PLACEHOLDER_TASK(AInputHandler)
JPOG_PLACEHOLDER_TASK(AVibrationManager)
JPOG_PLACEHOLDER_TASK(ARootStateController)
JPOG_PLACEHOLDER_TASK(ASoundManager)
JPOG_PLACEHOLDER_TASK(AMissionController)

#undef JPOG_PLACEHOLDER_TASK
