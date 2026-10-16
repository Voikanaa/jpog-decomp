#include "game/AApplication.h"
#include "game/CrashHandler.h"

#include <windows.h>
#include <cstdlib>

// Mirrors the game's WinMain (SimJP.exe 0x484330): single-instance mutex, then create and run
// the application. The original also performs a CD check here, which the reimplementation omits.
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	InstallCrashHandler();

	HANDLE mutex = CreateMutexA(nullptr, TRUE, "JPOPERATIONGENESIS");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		return 0;

	if (g_oTheApp.Create("Jurassic Park: Operation Genesis - (c) Blue Tongue Software", __argc, __argv))
		g_oTheApp.Execute();
	g_oTheApp.Destroy();

	if (mutex)
		CloseHandle(mutex);
	return 0;
}
