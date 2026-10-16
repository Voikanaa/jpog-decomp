#pragma once

// Access to the original game DLLs for differential tests (x86 builds only, since the
// originals are 32-bit). Modules are loaded from JPOG_ORIGINAL_DIR; when that directory or the
// module is missing, IsLoaded() is false and the differential tests are skipped.

#if defined(_M_IX86)

#include <float.h>
#include <string>

namespace jpog::test
{

class OriginalModule
{
public:
	explicit OriginalModule(const char* fileName);
	~OriginalModule();

	OriginalModule(const OriginalModule&) = delete;
	OriginalModule& operator=(const OriginalModule&) = delete;

	bool IsLoaded() const { return m_handle != nullptr; }

	// Looks up an export by its mangled name. Returns nullptr when it is not found.
	void* Find(const char* mangledName) const;

	// Typed lookup; the function pointer type must match the original calling convention.
	template <typename Fn> Fn Get(const char* mangledName) const { return reinterpret_cast<Fn>(Find(mangledName)); }

	static OriginalModule& Kernel();

private:
	void* m_handle = nullptr;
};

// Puts the x87 FPU into the state it has during gameplay: 24-bit precision, because the game
// creates its Direct3D 8 device without D3DCREATE_FPU_PRESERVE. Differential tests call the
// original code inside this scope; our SSE float code is unaffected by it.
class GamePrecisionScope
{
public:
	GamePrecisionScope()
	{
		_controlfp_s(&m_saved, 0, 0);
		unsigned int ignored;
		_controlfp_s(&ignored, _PC_24, _MCW_PC);
	}
	~GamePrecisionScope()
	{
		unsigned int ignored;
		_controlfp_s(&ignored, m_saved & _MCW_PC, _MCW_PC);
	}

	GamePrecisionScope(const GamePrecisionScope&) = delete;
	GamePrecisionScope& operator=(const GamePrecisionScope&) = delete;

private:
	unsigned int m_saved = 0;
};

} // namespace jpog::test

#define JPOG_HAS_ORIGINAL_MODULES 1
#else
#define JPOG_HAS_ORIGINAL_MODULES 0
#endif
