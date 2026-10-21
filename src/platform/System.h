#pragma once

#include <cstddef>

// Timer, dynamic libraries, files and system queries for the kernel: Win32 on Windows, POSIX
// elsewhere. Paths use the game's conventions (`\` or `/` separators, case-insensitive names);
// the POSIX backend converts separators and resolves names case-insensitively.
namespace jpog::platform
{

// --- High-resolution timer -----------------------------------------------------------------

long long PerformanceCounter();
long long PerformanceFrequency();

// --- Dynamic libraries ---------------------------------------------------------------------

void* LibraryLoad(const char* fileName);
void LibraryFree(void* library);
void* LibrarySymbol(void* library, const char* name);

// --- Files ---------------------------------------------------------------------------------

// The invalid file handle (INVALID_HANDLE_VALUE on Windows).
void* FileInvalid();

enum class FileAccess : unsigned int
{
	None = 0,
	Read = 1,
	Write = 2,
	ReadWrite = 3,
};

enum class FileOrigin
{
	Begin,
	End,
};

// Opens an existing file, or creates/truncates it with `create`. FileInvalid() on failure.
void* FileOpen(const char* path, FileAccess access, bool create);
void FileClose(void* file);
// `transferred` receives the number of bytes read or written (0 on failure).
bool FileRead(void* file, void* buffer, int size, int& transferred);
bool FileWrite(void* file, const void* buffer, int size, int& transferred);
// The new position, or -1 on failure (32-bit positions, as the original). After -1,
// FileSeekFailed() tells a failure from a position of 0xFFFFFFFF.
int FileSeek(void* file, int offset, FileOrigin origin);
bool FileSeekFailed();
// Last write time in Windows FILETIME units (100 ns since 1601).
unsigned long long FileWriteTime(void* file);
bool FileDelete(const char* path);
bool DirectoryMake(const char* path);
// The current working directory into `buffer`; false if it does not fit.
bool WorkingDirectory(char* buffer, std::size_t size);

// Directory listing of `pattern` (a directory followed by `\*`). Entries include "." and "..",
// as on Windows. FindFirst returns FileInvalid() when nothing is found.
struct FindEntry
{
	char name[260];
	bool directory;
};
void* FindFirst(const char* pattern, FindEntry& entry);
bool FindNext(void* find, FindEntry& entry);

// --- System --------------------------------------------------------------------------------

// Queries the OS version, directories, computer/user names and memory status and discards them,
// as TKernelInterface::DumpInfo does (nothing outside Windows).
void QuerySystemInfo();

} // namespace jpog::platform
