#include "toshi/kernel/TNativeFile.h"

#include "toshi/kernel/TMemory.h"

#include <cstdio>
#include <cwchar>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#undef CreateFile

namespace Toshi
{

namespace
{

constexpr int kSectorSize = 0x800;
constexpr int kSectorMask = ~(kSectorSize - 1);

int SetPointer(void* file, int offset, DWORD method)
{
	return static_cast<int>(SetFilePointer(file, offset, nullptr, method));
}

} // namespace

// --- TNativeFile ---------------------------------------------------------------------------

TNativeFile::TNativeFile(TNativeFileSystem* fileSystem)
    : TFile(fileSystem), m_hFile(INVALID_HANDLE_VALUE), m_iPosition(-1), m_iFilePointer(-1), m_iBufferStart(-1),
      m_iBufferSize(0), m_pReadBuffer(nullptr), m_pWriteBuffer(nullptr), m_iWriteBufferUsed(0), m_bWriteBuffered(true)
{
}

TNativeFile::TNativeFile(const TNativeFile& other)
    : TFile(other), m_hFile(other.m_hFile), m_iPosition(other.m_iPosition), m_iFilePointer(other.m_iFilePointer),
      m_iBufferStart(other.m_iBufferStart), m_iBufferSize(other.m_iBufferSize), m_pReadBuffer(other.m_pReadBuffer),
      m_pWriteBuffer(other.m_pWriteBuffer), m_iWriteBufferUsed(other.m_iWriteBufferUsed),
      m_bWriteBuffered(other.m_bWriteBuffered)
{
}

TNativeFile& TNativeFile::operator=(const TNativeFile& other)
{
	TFile::operator=(other);
	m_hFile = other.m_hFile;
	m_iPosition = other.m_iPosition;
	m_iFilePointer = other.m_iFilePointer;
	m_iBufferStart = other.m_iBufferStart;
	m_iBufferSize = other.m_iBufferSize;
	m_pReadBuffer = other.m_pReadBuffer;
	m_pWriteBuffer = other.m_pWriteBuffer;
	m_iWriteBufferUsed = other.m_iWriteBufferUsed;
	m_bWriteBuffered = other.m_bWriteBuffered;
	return *this;
}

TNativeFile::~TNativeFile()
{
}

TNativeFileSystem* TNativeFile::GetFileSystem() const
{
	return static_cast<TNativeFileSystem*>(TFile::GetFileSystem());
}

bool TNativeFile::Open(const TCString& path, unsigned int flags)
{
	const TCString internalPath = GetFileSystem()->MakeInternalPath(path);
	DWORD access = 0;
	if (flags & 1)
		access = GENERIC_READ;
	if (flags & 2)
		access |= GENERIC_WRITE;
	if (flags & 4)
		access = GENERIC_READ | GENERIC_WRITE;
	DWORD disposition = OPEN_EXISTING;
	if (flags & 8)
	{
		disposition = CREATE_ALWAYS;
		if (access == 0)
			access = GENERIC_WRITE;
	}
	void* handle = CreateFileA(internalPath.GetString(0), access, 0, nullptr, disposition, 0, nullptr);
	if (handle == INVALID_HANDLE_VALUE)
		return false;
	m_hFile = handle;
	m_iPosition = 0;
	m_iFilePointer = 0;
	m_iBufferStart = -1;
	m_iBufferSize = 0;
	if ((flags & 0x10) == 0)
		m_pReadBuffer = static_cast<unsigned char*>(tmalloc(kSectorSize, nullptr, -1));
	m_bWriteBuffered = false;
	return true;
}

void TNativeFile::Close()
{
	FlushWriteBuffer();
	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
	m_iPosition = -1;
	m_iFilePointer = -1;
	m_iBufferStart = -1;
	m_iBufferSize = 0;
	if (m_pReadBuffer != nullptr)
	{
		tfree(m_pReadBuffer);
		m_pReadBuffer = nullptr;
	}
	if (m_pWriteBuffer != nullptr)
	{
		tfree(m_pWriteBuffer);
		m_pWriteBuffer = nullptr;
	}
}

int TNativeFile::ReadUnbuffered(void* buffer, int size)
{
	if (m_iPosition != m_iFilePointer)
	{
		m_iFilePointer = SetPointer(m_hFile, m_iPosition, FILE_BEGIN);
		m_iPosition = m_iFilePointer;
	}
	DWORD read = 0;
	ReadFile(m_hFile, buffer, static_cast<DWORD>(size), &read, nullptr);
	m_iFilePointer += static_cast<int>(read);
	m_iPosition = m_iFilePointer;
	return static_cast<int>(read);
}

bool TNativeFile::LoadBuffer(int sectorStart)
{
	if (sectorStart == m_iBufferStart)
		return true;
	if (sectorStart != m_iFilePointer)
	{
		m_iFilePointer = SetPointer(m_hFile, sectorStart, FILE_BEGIN);
		if (m_iFilePointer != sectorStart)
			return false;
	}
	DWORD read = 0;
	if (!ReadFile(m_hFile, m_pReadBuffer, kSectorSize, &read, nullptr))
		return false;
	m_iFilePointer += static_cast<int>(read);
	m_iBufferSize = static_cast<int>(read);
	m_iBufferStart = sectorStart;
	return true;
}

int TNativeFile::FlushWriteBuffer()
{
	if (m_iPosition != m_iFilePointer)
	{
		m_iFilePointer = SetPointer(m_hFile, m_iPosition, FILE_BEGIN);
		m_iPosition = m_iFilePointer;
	}
	DWORD written = 0;
	WriteFile(m_hFile, m_pWriteBuffer, static_cast<DWORD>(m_iWriteBufferUsed), &written, nullptr);
	m_iFilePointer += static_cast<int>(written);
	m_iPosition = m_iFilePointer;
	m_iWriteBufferUsed = 0;
	return static_cast<int>(written);
}

// Bytes before the sector of the end position are copied from the buffer (if it holds the
// current sector) and read directly; the tail comes from the buffered end sector.
int TNativeFile::Read(void* buffer, int size)
{
	if (size < 1)
		return 0;
	if (m_pReadBuffer == nullptr)
		return ReadUnbuffered(buffer, size);

	auto* out = static_cast<unsigned char*>(buffer);
	int total = 0;
	int sector = m_iPosition & kSectorMask;
	const int endSector = (m_iPosition + size) & kSectorMask;
	if (sector != endSector)
	{
		if (sector == m_iBufferStart)
		{
			const int available = m_iBufferSize - (m_iPosition - sector);
			if (available > 0)
			{
				const unsigned char* in = m_pReadBuffer + (m_iPosition - sector);
				for (int i = 0; i < available; ++i)
					*out++ = in[i];
				m_iPosition += available;
				total = available;
			}
		}
		const int direct = endSector - m_iPosition;
		sector = endSector;
		if (direct > 0)
		{
			const int read = ReadUnbuffered(out, direct);
			out += read;
			total += read;
			if (read != direct)
				return total;
		}
	}
	if (total != size)
	{
		if (!LoadBuffer(sector))
			return total;
		int count = size - total;
		const int available = m_iBufferSize - (m_iPosition - sector);
		if (available < count)
			count = available;
		if (count > 0)
		{
			const unsigned char* in = m_pReadBuffer + (m_iPosition - sector);
			for (int i = 0; i < count; ++i)
				*out++ = in[i];
			m_iPosition += count;
			total += count;
		}
	}
	return total;
}

int TNativeFile::Write(const void* buffer, int size)
{
	if (m_iPosition != m_iFilePointer)
	{
		m_iFilePointer = SetPointer(m_hFile, m_iPosition, FILE_BEGIN);
		m_iPosition = m_iFilePointer;
	}
	if (m_bWriteBuffered)
	{
		if (m_iWriteBufferUsed + size >= kSectorSize)
			FlushWriteBuffer();
		if (size >= kSectorSize)
		{
			DWORD written = 0;
			WriteFile(m_hFile, buffer, static_cast<DWORD>(size), &written, nullptr);
			m_iFilePointer += static_cast<int>(written);
			m_iPosition = m_iFilePointer;
			return static_cast<int>(written);
		}
		const auto* in = static_cast<const unsigned char*>(buffer);
		for (int i = 0; i < size; ++i)
			m_pWriteBuffer[m_iWriteBufferUsed + i] = in[i];
		m_iWriteBufferUsed += size;
		return size;
	}
	DWORD written = 0;
	if (!WriteFile(m_hFile, buffer, static_cast<DWORD>(size), &written, nullptr))
		return 0;
	m_iFilePointer += static_cast<int>(written);
	m_iPosition = m_iFilePointer;
	return static_cast<int>(written);
}

bool TNativeFile::Seek(int offset, TSEEK origin)
{
	switch (origin)
	{
	case TSEEK_SET:
		m_iPosition = offset < 0 ? 0 : offset;
		break;
	case TSEEK_CUR:
		m_iPosition = m_iPosition + offset < 0 ? 0 : m_iPosition + offset;
		break;
	case TSEEK_END:
		m_iFilePointer = SetPointer(m_hFile, offset, FILE_END);
		if (m_iFilePointer == -1 && GetLastError() != NO_ERROR)
			return false;
		m_iPosition = m_iFilePointer;
		break;
	}
	return true;
}

int TNativeFile::Tell()
{
	return m_iPosition;
}

int TNativeFile::GetSize()
{
	m_iFilePointer = SetPointer(m_hFile, 0, FILE_END);
	return m_iFilePointer;
}

unsigned __int64 TNativeFile::GetDate()
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return 0;
	FILETIME created, accessed, written;
	GetFileTime(m_hFile, &created, &accessed, &written);
	return (static_cast<unsigned __int64>(written.dwHighDateTime) << 32) | written.dwLowDateTime;
}

int TNativeFile::GetCChar()
{
	if (m_pReadBuffer != nullptr)
	{
		const int sector = m_iPosition & kSectorMask;
		const int offset = m_iPosition - sector;
		if (sector == m_iBufferStart && offset <= m_iBufferSize - 1)
		{
			++m_iPosition;
			return m_pReadBuffer[offset];
		}
	}
	unsigned char character = 0;
	if (Read(&character, 1) != 1)
		return -1;
	return character;
}

int TNativeFile::GetWChar()
{
	if (m_pReadBuffer != nullptr)
	{
		const int sector = m_iPosition & kSectorMask;
		const int offset = m_iPosition - sector;
		if (sector == m_iBufferStart && offset <= m_iBufferSize - 2)
		{
			m_iPosition += 2;
			return m_pReadBuffer[offset] | (m_pReadBuffer[offset + 1] << 8);
		}
	}
	unsigned short character = 0;
	if (Read(&character, 2) != 2)
		return -1;
	return character;
}

int TNativeFile::PutCChar(char character)
{
	if (Write(&character, 1) != 1)
		return -1;
	return character;
}

int TNativeFile::PutWChar(unsigned short character)
{
	if (Write(&character, 2) != 2)
		return -1;
	return character;
}

int TNativeFile::VCPrintf(const char* format, char* args)
{
	char buffer[0x200];
	int length = _vsnprintf(buffer, sizeof(buffer), format, args);
	if (length < 0)
		length = 0x200;
	return Write(buffer, length);
}

int TNativeFile::VWPrintf(const unsigned short* format, char* args)
{
	wchar_t buffer[0x200];
	int length = _vsnwprintf(buffer, 0x200, reinterpret_cast<const wchar_t*>(format), args);
	if (length < 0)
		length = 0x200;
	const int written = Write(buffer, length * 2);
	return written < 0 ? written : written >> 1;
}

// --- TNativeFileSystem ---------------------------------------------------------------------

TNativeFileSystem::TNativeFileSystem(const char* name) : TFileSystem(name), m_hFind(INVALID_HANDLE_VALUE)
{
	TFileManager::GetFileManager()->MountFileSystem(this);
}

TNativeFileSystem::TNativeFileSystem(const TNativeFileSystem& other) : TFileSystem(other), m_hFind(other.m_hFind)
{
}

TNativeFileSystem& TNativeFileSystem::operator=(TNativeFileSystem& other)
{
	TFileSystem::operator=(other);
	m_hFind = other.m_hFind;
	return *this;
}

TNativeFileSystem::~TNativeFileSystem()
{
	TFileManager::GetFileManager()->UnmountFileSystem(this);
}

TFile* TNativeFileSystem::CreateFile(const TCString& path, unsigned int flags)
{
	auto* file = new TNativeFile(this);
	if (file->Open(path, flags))
		return file;
	delete file;
	return nullptr;
}

void TNativeFileSystem::DestroyFile(TFile* file)
{
	if (file == nullptr)
		return;
	auto* nativeFile = static_cast<TNativeFile*>(file);
	nativeFile->Close();
	delete nativeFile;
}

bool TNativeFileSystem::RemoveFile(const TCString& path)
{
	return DeleteFileA(path.GetString(0)) != FALSE;
}

TCString TNativeFileSystem::MakeInternalPath(const TCString& path) const
{
	TCString result(GetPrefix());
	const int length = result.Length();
	if (length > 0 && (result[length - 1] == '/' || result[length - 1] == '\\'))
		result.Truncate(length - 1);
	result += TFileManager::GetFileManager()->MakeAbsolutePath(path);
	for (int i = 0; i < result.Length(); ++i)
		if (result[i] == '/')
			result[i] = '\\';
	return result;
}

namespace
{

// Applies the file/directory filter to a find result.
bool Accept(const WIN32_FIND_DATAA& data, unsigned int flags, TCString& name)
{
	const bool directory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	if (directory ? (flags & 2) == 0 : (flags & 1) == 0)
		return false;
	name = data.cFileName;
	return true;
}

} // namespace

bool TNativeFileSystem::GetFirstFile(const TCString& path, TCString& name, unsigned int flags)
{
	TCString pattern(path);
	const TCString last = path.Mid(path.Length() - 1, 1);
	const bool endsWithSeparator = last.Compare("\\", -1) == 0 || last.Compare("/", -1) == 0;
	pattern.Concat(endsWithSeparator ? "*" : "\\*", -1);
	WIN32_FIND_DATAA data;
	m_hFind = FindFirstFileA(pattern.GetString(0), &data);
	return m_hFind != INVALID_HANDLE_VALUE && Accept(data, flags, name);
}

bool TNativeFileSystem::GetNextFile(TCString& name, unsigned int flags)
{
	if (m_hFind == INVALID_HANDLE_VALUE)
		return false;
	WIN32_FIND_DATAA data;
	if (!FindNextFileA(m_hFind, &data))
	{
		m_hFind = INVALID_HANDLE_VALUE;
		return false;
	}
	return Accept(data, flags, name);
}

bool TNativeFileSystem::MakeDirectory(const TCString& path)
{
	return CreateDirectoryA(path.GetString(0), nullptr) != FALSE;
}

} // namespace Toshi
