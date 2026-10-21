#include <windows.h>

// Phase 1 will install hooks here. For now the DLL only proves it can be built
// for x86 and loaded into the original process (milestone M1).
BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(module);
	return TRUE;
}
