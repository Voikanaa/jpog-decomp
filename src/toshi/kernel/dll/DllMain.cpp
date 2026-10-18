// Entry point of our drop-in TKernelInterface.dll. Like the original, it only disables thread
// notifications; everything else is static initialisation.
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(instance);
	return TRUE;
}
