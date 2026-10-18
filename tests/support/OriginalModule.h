#pragma once

// Access to the original game DLLs for differential tests (x86 builds only, since the
// originals are 32-bit). Modules are loaded from JPOG_ORIGINAL_DIR; when that directory or the
// module is missing, IsLoaded() is false and the differential tests are skipped.

#if defined(_M_IX86)

#include <cstddef>
#include <filesystem>
#include <float.h>
#include <string>
#include <type_traits>

namespace jpog::test
{

// JPOG_ORIGINAL_DIR (or the environment override): the original install with the DLLs and Data.
std::filesystem::path OriginalModuleDirectory();

// Callable wrapper for an original __thiscall member function: R Class::fn(Args...).
// MSVC does not allow __thiscall on plain function pointer types, so the call goes through
// __fastcall, which passes the first argument in ECX (`this`) and the second in EDX (unused);
// both conventions pass the rest on the stack and have the callee clean it up. Members that
// return a class by value take the hidden result pointer as their first stack argument.
template <typename R, typename... Args> struct Method
{
	void* address = nullptr;

	explicit operator bool() const { return address != nullptr; }
	bool operator==(std::nullptr_t) const { return address == nullptr; }

	R operator()(const void* self, Args... args) const
	{
		using Fn = R(__fastcall*)(const void*, void*, Args...);
		return reinterpret_cast<Fn>(address)(self, nullptr, args...);
	}
};

template <typename T> struct IsMethod : std::false_type
{
};
template <typename R, typename... Args> struct IsMethod<Method<R, Args...>> : std::true_type
{
};

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
	template <typename Fn> Fn Get(const char* mangledName) const
	{
		if constexpr (IsMethod<Fn>::value)
			return Fn{Find(mangledName)};
		else
			return reinterpret_cast<Fn>(Find(mangledName));
	}

	static OriginalModule& Kernel();
	static OriginalModule& PPropertyParser();

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
