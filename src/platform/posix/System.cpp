#include "platform/System.h"

#include <cerrno>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <string>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

namespace jpog::platform
{

namespace
{

bool Exists(const std::string& path)
{
	struct stat info;
	return stat(path.c_str(), &info) == 0;
}

// Converts a game path to a native one: `\` becomes `/`, and each component that does not
// exist with its exact name is matched case-insensitively against its directory. Components
// that match nothing (e.g. a file about to be created) are kept as they are.
std::string NativePath(const char* path)
{
	std::string converted(path);
	for (char& c : converted)
		if (c == '\\')
			c = '/';
	if (converted.empty() || Exists(converted))
		return converted;

	std::string result;
	std::size_t position = 0;
	if (converted[0] == '/')
	{
		result = "/";
		position = 1;
	}
	bool resolving = true;
	while (position <= converted.size())
	{
		std::size_t end = converted.find('/', position);
		if (end == std::string::npos)
			end = converted.size();
		const std::string component = converted.substr(position, end - position);
		position = end + 1;
		if (component.empty())
			continue;
		if (!result.empty() && result.back() != '/')
			result += '/';
		const std::string directory = result.empty() ? std::string(".") : result;
		std::string name = component;
		if (resolving && !Exists(result + component))
		{
			resolving = false;
			if (DIR* dir = opendir(directory.c_str()))
			{
				while (const dirent* entry = readdir(dir))
					if (strcasecmp(entry->d_name, component.c_str()) == 0)
					{
						name = entry->d_name;
						resolving = true;
						break;
					}
				closedir(dir);
			}
		}
		result += name;
	}
	return result;
}

struct File
{
	int descriptor;
};

thread_local bool t_seekFailed = false;

} // namespace

// --- Timer -----------------------------------------------------------------------------------

long long PerformanceCounter()
{
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return static_cast<long long>(now.tv_sec) * 1000000000LL + now.tv_nsec;
}

long long PerformanceFrequency()
{
	return 1000000000LL;
}

// --- Dynamic libraries -----------------------------------------------------------------------

void* LibraryLoad(const char* fileName)
{
	return dlopen(NativePath(fileName).c_str(), RTLD_NOW | RTLD_LOCAL);
}

void LibraryFree(void* library)
{
	dlclose(library);
}

void* LibrarySymbol(void* library, const char* name)
{
	return dlsym(library, name);
}

// --- Files -----------------------------------------------------------------------------------

void* FileInvalid()
{
	return nullptr;
}

void* FileOpen(const char* path, FileAccess access, bool create)
{
	int flags = O_RDONLY;
	if (access == FileAccess::Write)
		flags = O_WRONLY;
	else if (access == FileAccess::ReadWrite)
		flags = O_RDWR;
	if (create)
		flags |= O_CREAT | O_TRUNC;
	const int descriptor = open(NativePath(path).c_str(), flags | O_CLOEXEC, 0644);
	if (descriptor < 0)
		return FileInvalid();
	return new File{descriptor};
}

void FileClose(void* file)
{
	if (file == FileInvalid())
		return;
	close(static_cast<File*>(file)->descriptor);
	delete static_cast<File*>(file);
}

bool FileRead(void* file, void* buffer, int size, int& transferred)
{
	transferred = 0;
	if (file == FileInvalid() || size < 0)
		return false;
	auto* out = static_cast<unsigned char*>(buffer);
	while (transferred < size)
	{
		const ssize_t count =
		    read(static_cast<File*>(file)->descriptor, out + transferred, static_cast<std::size_t>(size - transferred));
		if (count < 0 && errno == EINTR)
			continue;
		if (count < 0)
			return false;
		if (count == 0)
			break;
		transferred += static_cast<int>(count);
	}
	return true;
}

bool FileWrite(void* file, const void* buffer, int size, int& transferred)
{
	transferred = 0;
	if (file == FileInvalid() || size < 0)
		return false;
	const auto* in = static_cast<const unsigned char*>(buffer);
	while (transferred < size)
	{
		const ssize_t count =
		    write(static_cast<File*>(file)->descriptor, in + transferred, static_cast<std::size_t>(size - transferred));
		if (count < 0 && errno == EINTR)
			continue;
		if (count <= 0)
			return false;
		transferred += static_cast<int>(count);
	}
	return true;
}

int FileSeek(void* file, int offset, FileOrigin origin)
{
	t_seekFailed = true;
	if (file == FileInvalid())
		return -1;
	const off_t position =
	    lseek(static_cast<File*>(file)->descriptor, offset, origin == FileOrigin::End ? SEEK_END : SEEK_SET);
	if (position < 0)
		return -1;
	t_seekFailed = false;
	return static_cast<int>(position);
}

bool FileSeekFailed()
{
	return t_seekFailed;
}

unsigned long long FileWriteTime(void* file)
{
	struct stat info;
	if (file == FileInvalid() || fstat(static_cast<File*>(file)->descriptor, &info) != 0)
		return 0;
#if defined(__APPLE__)
	const timespec& written = info.st_mtimespec;
#else
	const timespec& written = info.st_mtim;
#endif
	// FILETIME counts 100 ns intervals from 1601-01-01.
	constexpr unsigned long long kSecondsFrom1601To1970 = 11644473600ULL;
	return (static_cast<unsigned long long>(written.tv_sec) + kSecondsFrom1601To1970) * 10000000ULL +
	       static_cast<unsigned long long>(written.tv_nsec) / 100;
}

bool FileDelete(const char* path)
{
	return unlink(NativePath(path).c_str()) == 0;
}

bool DirectoryMake(const char* path)
{
	return mkdir(NativePath(path).c_str(), 0755) == 0;
}

bool WorkingDirectory(char* buffer, std::size_t size)
{
	return getcwd(buffer, size) != nullptr;
}

// --- Directory listing -----------------------------------------------------------------------

namespace
{

struct Find
{
	DIR* dir;
	std::string directory;
};

bool Next(Find& find, FindEntry& entry)
{
	const dirent* item = readdir(find.dir);
	if (item == nullptr)
		return false;
	std::strncpy(entry.name, item->d_name, sizeof(entry.name) - 1);
	entry.name[sizeof(entry.name) - 1] = '\0';
	struct stat info;
	entry.directory = stat((find.directory + "/" + item->d_name).c_str(), &info) == 0 && S_ISDIR(info.st_mode);
	return true;
}

} // namespace

void* FindFirst(const char* pattern, FindEntry& entry)
{
	std::string directory(pattern);
	if (!directory.empty() && directory.back() == '*')
		directory.pop_back();
	while (!directory.empty() && (directory.back() == '/' || directory.back() == '\\'))
		directory.pop_back();
	directory = directory.empty() ? std::string(".") : NativePath(directory.c_str());
	DIR* dir = opendir(directory.c_str());
	if (dir == nullptr)
		return FileInvalid();
	auto* find = new Find{dir, directory};
	if (!Next(*find, entry))
	{
		closedir(dir);
		delete find;
		return FileInvalid();
	}
	return find;
}

// The listing is released when it runs out (the caller drops the handle then).
bool FindNext(void* find, FindEntry& entry)
{
	auto* state = static_cast<Find*>(find);
	if (Next(*state, entry))
		return true;
	closedir(state->dir);
	delete state;
	return false;
}

// --- System ----------------------------------------------------------------------------------

void QuerySystemInfo()
{
}

} // namespace jpog::platform
