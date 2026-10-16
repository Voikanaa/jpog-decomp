#include "game/CrashHandler.h"

#include <windows.h>
#include <cstdio>
#include <dbghelp.h>

namespace
{

// "TKernelInterface.dll+0x1234", or the raw address if it is not inside a loaded module.
void DescribeAddress(DWORD64 address, char* out, size_t size)
{
	HMODULE module = nullptr;
	char path[MAX_PATH] = "";
	if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                       reinterpret_cast<LPCSTR>(static_cast<uintptr_t>(address)), &module) &&
	    GetModuleFileNameA(module, path, sizeof(path)))
	{
		const char* name = strrchr(path, '\\');
		std::snprintf(out, size, "%s+0x%llx", name ? name + 1 : path,
		              static_cast<unsigned long long>(address - reinterpret_cast<uintptr_t>(module)));
		return;
	}
	std::snprintf(out, size, "0x%llx", static_cast<unsigned long long>(address));
}

LONG WINAPI WriteCrashReport(EXCEPTION_POINTERS* info)
{
	FILE* log = nullptr;
	if (fopen_s(&log, "jpog_crash.log", "w") != 0 || !log)
		return EXCEPTION_CONTINUE_SEARCH;

	const EXCEPTION_RECORD* record = info->ExceptionRecord;
	char where[MAX_PATH + 32];
	DescribeAddress(reinterpret_cast<uintptr_t>(record->ExceptionAddress), where, sizeof(where));
	std::fprintf(log, "Exception 0x%08lx at %s\n", record->ExceptionCode, where);
	if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2)
		std::fprintf(log, "  %s address 0x%llx\n", record->ExceptionInformation[0] ? "writing" : "reading",
		             static_cast<unsigned long long>(record->ExceptionInformation[1]));

	// A call through a null or garbage function pointer leaves no usable PC; the return address on
	// top of the stack then identifies the caller.
	CONTEXT context = *info->ContextRecord;
#if defined(_M_IX86)
	HMODULE faultingModule = nullptr;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                        static_cast<LPCSTR>(record->ExceptionAddress), &faultingModule))
	{
		const DWORD returnAddress = *reinterpret_cast<const DWORD*>(static_cast<uintptr_t>(context.Esp));
		DescribeAddress(returnAddress, where, sizeof(where));
		std::fprintf(log, "  called from %s (return address on the stack)\n", where);
		context.Eip = returnAddress;
		context.Esp += 4;
	}
#endif

	// Walk the stack from the faulting context.
	STACKFRAME64 frame{};
	DWORD machine;
#if defined(_M_IX86)
	machine = IMAGE_FILE_MACHINE_I386;
	frame.AddrPC.Offset = context.Eip;
	frame.AddrFrame.Offset = context.Ebp;
	frame.AddrStack.Offset = context.Esp;
#else
	machine = IMAGE_FILE_MACHINE_AMD64;
	frame.AddrPC.Offset = context.Rip;
	frame.AddrFrame.Offset = context.Rbp;
	frame.AddrStack.Offset = context.Rsp;
#endif
	frame.AddrPC.Mode = frame.AddrFrame.Mode = frame.AddrStack.Mode = AddrModeFlat;

	HANDLE process = GetCurrentProcess();
	SymInitialize(process, nullptr, TRUE);
	std::fprintf(log, "Stack:\n");
	for (int depth = 0; depth < 64; ++depth)
	{
		if (!StackWalk64(machine, process, GetCurrentThread(), &frame, &context, nullptr, SymFunctionTableAccess64,
		                 SymGetModuleBase64, nullptr) ||
		    frame.AddrPC.Offset == 0)
			break;
		DescribeAddress(frame.AddrPC.Offset, where, sizeof(where));
		std::fprintf(log, "  %2d %s\n", depth, where);
	}
	SymCleanup(process);
	std::fclose(log);
	return EXCEPTION_CONTINUE_SEARCH;
}

} // namespace

void InstallCrashHandler()
{
	SetUnhandledExceptionFilter(WriteCrashReport);
}
