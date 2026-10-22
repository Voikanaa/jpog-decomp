#include "platform/System.h"

#include <windows.h>
#include <direct.h>

namespace jpog::platform
{

long long PerformanceCounter()
{
	LARGE_INTEGER value;
	QueryPerformanceCounter(&value);
	return value.QuadPart;
}

long long PerformanceFrequency()
{
	LARGE_INTEGER value;
	QueryPerformanceFrequency(&value);
	return value.QuadPart;
}

void* LibraryLoad(const char* fileName)
{
	return LoadLibraryA(fileName);
}

void LibraryFree(void* library)
{
	FreeLibrary(static_cast<HMODULE>(library));
}

void* LibrarySymbol(void* library, const char* name)
{
	return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(library), name));
}

void* FileInvalid()
{
	return INVALID_HANDLE_VALUE;
}

void* FileOpen(const char* path, FileAccess access, bool create)
{
	DWORD desired = 0;
	if ((static_cast<unsigned int>(access) & static_cast<unsigned int>(FileAccess::Read)) != 0)
		desired |= GENERIC_READ;
	if ((static_cast<unsigned int>(access) & static_cast<unsigned int>(FileAccess::Write)) != 0)
		desired |= GENERIC_WRITE;
	return CreateFileA(path, desired, 0, nullptr, create ? CREATE_ALWAYS : OPEN_EXISTING, 0, nullptr);
}

void FileClose(void* file)
{
	CloseHandle(file);
}

bool FileRead(void* file, void* buffer, int size, int& transferred)
{
	DWORD read = 0;
	const BOOL result = ReadFile(file, buffer, static_cast<DWORD>(size), &read, nullptr);
	transferred = static_cast<int>(read);
	return result != FALSE;
}

bool FileWrite(void* file, const void* buffer, int size, int& transferred)
{
	DWORD written = 0;
	const BOOL result = WriteFile(file, buffer, static_cast<DWORD>(size), &written, nullptr);
	transferred = static_cast<int>(written);
	return result != FALSE;
}

int FileSeek(void* file, int offset, FileOrigin origin)
{
	return static_cast<int>(SetFilePointer(file, offset, nullptr, origin == FileOrigin::End ? FILE_END : FILE_BEGIN));
}

bool FileSeekFailed()
{
	return GetLastError() != NO_ERROR;
}

unsigned long long FileWriteTime(void* file)
{
	FILETIME created, accessed, written;
	GetFileTime(file, &created, &accessed, &written);
	return (static_cast<unsigned long long>(written.dwHighDateTime) << 32) | written.dwLowDateTime;
}

bool FileDelete(const char* path)
{
	return DeleteFileA(path) != FALSE;
}

bool DirectoryMake(const char* path)
{
	return CreateDirectoryA(path, nullptr) != FALSE;
}

bool WorkingDirectory(char* buffer, std::size_t size)
{
	return _getcwd(buffer, static_cast<int>(size)) != nullptr;
}

namespace
{

void ToEntry(const WIN32_FIND_DATAA& data, FindEntry& entry)
{
	lstrcpynA(entry.name, data.cFileName, sizeof(entry.name));
	entry.directory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

} // namespace

void* FindFirst(const char* pattern, FindEntry& entry)
{
	WIN32_FIND_DATAA data;
	HANDLE find = FindFirstFileA(pattern, &data);
	if (find != INVALID_HANDLE_VALUE)
		ToEntry(data, entry);
	return find;
}

// The original never closes the search handle.
bool FindNext(void* find, FindEntry& entry)
{
	WIN32_FIND_DATAA data;
	if (!FindNextFileA(find, &data))
		return false;
	ToEntry(data, entry);
	return true;
}

void QuerySystemInfo()
{
	// The extended structure first, then the basic one (the buffer must hold the larger).
	OSVERSIONINFOEXA version = {};
	version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
	auto* basic = reinterpret_cast<OSVERSIONINFOA*>(&version);
#pragma warning(suppress : 4996) // GetVersionExA is what the original calls
	if (!GetVersionExA(basic))
	{
		version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
#pragma warning(suppress : 4996)
		GetVersionExA(basic);
	}
	char windowsDirectory[0x104];
	GetWindowsDirectoryA(windowsDirectory, sizeof(windowsDirectory));
	char systemDirectory[0x104];
	GetSystemDirectoryA(systemDirectory, sizeof(systemDirectory));
	char computerName[0x10];
	DWORD computerNameLength = sizeof(computerName);
	GetComputerNameA(computerName, &computerNameLength);
	char userName[0x100];
	DWORD userNameLength = sizeof(userName);
	GetUserNameA(userName, &userNameLength);
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	MEMORYSTATUS memory = {};
	memory.dwLength = sizeof(memory);
	GlobalMemoryStatus(&memory);
}

} // namespace jpog::platform
