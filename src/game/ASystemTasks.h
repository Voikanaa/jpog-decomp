#pragma once

#include "common/abi.h"
#include "toshi/kernel/TScheduler.h"

// The game's subsystem tasks (SimJP.exe) that ARootTask allocates and creates. Placeholders for
// now: each has the original class name and its place in the task tree, but no behaviour yet
// (issue #21); each moves to its own file when it is reimplemented. Their TClass versions are
// not recovered (the game builds its TClass objects in code we do not read), so 0x10000 is
// assumed. AInputHandler's name is inferred: its TClass is not exported and "AInputHandler" is
// the only matching class name in the exe.
#define JPOG_PLACEHOLDER_TASK(Name) \
	class Name : public Toshi::TTask \
	{ \
	public: \
		Toshi::TClass& GetClass() const override; \
		static Toshi::TClass m_sClass; \
\
	private: \
		static Toshi::TObject* TSTDCALL CreateObject(); \
		static Toshi::TObject* TSTDCALL CreateObjectInPlace(void* memory); \
		static void TSTDCALL InitialiseStatic(); \
		static void TSTDCALL DeinitialiseStatic(); \
	};

JPOG_PLACEHOLDER_TASK(AToshiAnimationSystem) // animation, under ARootTask
JPOG_PLACEHOLDER_TASK(ACameraManager)        // cameras, under Update3
JPOG_PLACEHOLDER_TASK(AGUISystem)            // the game's GUI, under Update3
JPOG_PLACEHOLDER_TASK(AInputHandler)         // input mapping, under Input
JPOG_PLACEHOLDER_TASK(AVibrationManager)     // controller vibration, under Input (created inactive)
JPOG_PLACEHOLDER_TASK(ARootStateController)  // the game state machine, under ARootTask
JPOG_PLACEHOLDER_TASK(ASoundManager)         // sound, under ARootTask
JPOG_PLACEHOLDER_TASK(AMissionController)    // missions, under ARootTask

#undef JPOG_PLACEHOLDER_TASK
