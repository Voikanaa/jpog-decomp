#include "support/OriginalModule.h"

#if JPOG_HAS_ORIGINAL_MODULES

#include <windows.h>

#include <cstdlib>
#include <filesystem>

namespace jpog::test
{

namespace
{

std::filesystem::path OriginalDirectory()
{
	if (const char* overrideDir = std::getenv("JPOG_ORIGINAL_DIR"); overrideDir && *overrideDir)
		return overrideDir;
	return JPOG_ORIGINAL_DIR;
}

} // namespace

OriginalModule::OriginalModule(const char* fileName)
{
	const std::filesystem::path path = OriginalDirectory() / fileName;
	if (!std::filesystem::exists(path))
		return;

	// Resolve the module's own dependencies (e.g. libmmd.dll) from the game directory.
	m_handle = LoadLibraryExW(path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
}

OriginalModule::~OriginalModule()
{
	if (m_handle)
		FreeLibrary(static_cast<HMODULE>(m_handle));
}

void* OriginalModule::Find(const char* mangledName) const
{
	if (!m_handle)
		return nullptr;
	return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(m_handle), mangledName));
}

OriginalModule& OriginalModule::Kernel()
{
	static OriginalModule s_module("TKernelInterface.dll");
	return s_module;
}

} // namespace jpog::test

#endif
