#include "toshi/kernel/file/TMemFile.h"

#include "toshi/kernel/TSystem.h"
#include "toshi/kernel/memory/TMemory.h"

#include <cstdio>
#include <cwchar>

namespace Toshi
{

// --- TMemFile ------------------------------------------------------------------------------

TMemFile::TMemFile(TMemFileSystem* fileSystem) : TFile(fileSystem), m_pData(nullptr), m_pCursor(nullptr), m_iSize(0)
{
}

TMemFile::TMemFile(const TMemFile& other)
    : TFile(other), m_pData(other.m_pData), m_pCursor(other.m_pCursor), m_iSize(other.m_iSize)
{
}

TMemFile& TMemFile::operator=(const TMemFile& other)
{
	TFile::operator=(other);
	m_pData = other.m_pData;
	m_pCursor = other.m_pCursor;
	m_iSize = other.m_iSize;
	return *this;
}

TMemFile::~TMemFile()
{
}

bool TMemFile::Open(const TCString& path, unsigned int flags)
{
	TFile* file = TFileManager::GetFileManager()->CreateFile(path, flags);
	if (file == nullptr)
		return true;
	m_iSize = file->GetSize();
	if (m_iSize <= 0)
		return true;
	m_pData = static_cast<unsigned char*>(tmalloc(static_cast<unsigned int>(m_iSize), nullptr, -1));
	m_pCursor = m_pData;
	return file->Read(m_pData, m_iSize) == m_iSize;
}

bool TMemFile::Open(TFile* file)
{
	if (file == nullptr)
		return true;
	m_iSize = file->GetSize();
	if (m_iSize <= 0)
		return true;
	m_pData = static_cast<unsigned char*>(tmalloc(static_cast<unsigned int>(m_iSize), nullptr, -1));
	m_pCursor = m_pData;
	file->Seek(0, TSEEK_SET);
	return file->Read(m_pData, m_iSize) == m_iSize;
}

void TMemFile::Close()
{
	if (m_pData != nullptr)
	{
		tfree(m_pData);
		m_pData = nullptr;
	}
	m_iSize = 0;
	m_pCursor = nullptr;
}

void TMemFile::DumpFile()
{
	Seek(0, TSEEK_SET);
}

void TMemFile::DebugStats()
{
}

void TMemFile::ClearStats()
{
}

int TMemFile::Read(void* buffer, int size)
{
	if (size > 0 && Tell() + size <= m_iSize)
	{
		TSystem::MemCopy(buffer, m_pCursor, static_cast<unsigned int>(size));
		m_pCursor += size;
		return size;
	}
	return -1;
}

int TMemFile::Write(const void*, int size)
{
	return size;
}

bool TMemFile::Seek(int offset, TSEEK origin)
{
	switch (origin)
	{
	case TSEEK_SET:
		m_pCursor = offset > m_iSize ? m_pData : m_pData + offset;
		break;
	case TSEEK_CUR:
		m_pCursor = Tell() + offset > m_iSize ? m_pData : m_pCursor + offset;
		break;
	case TSEEK_END:
		m_pCursor = m_pData + m_iSize;
		break;
	}
	return true;
}

int TMemFile::Tell()
{
	return static_cast<int>(m_pCursor - m_pData);
}

int TMemFile::GetSize()
{
	return m_iSize;
}

int TMemFile::GetCChar()
{
	return -1;
}

int TMemFile::GetWChar()
{
	return 0xFFFF;
}

int TMemFile::PutCChar(char character)
{
	if (Write(&character, 1) != 1)
		return -1;
	return character;
}

int TMemFile::PutWChar(unsigned short character)
{
	if (Write(&character, 2) != 2)
		return -1;
	return character;
}

int TMemFile::VCPrintf(const char* format, char* args)
{
	char buffer[0x200];
	int length = _vsnprintf(buffer, sizeof(buffer), format, args);
	if (length < 0)
		length = 0x200;
	return Write(buffer, length);
}

int TMemFile::VWPrintf(const unsigned short* format, char* args)
{
	wchar_t buffer[0x200];
	int length = _vsnwprintf(buffer, 0x200, reinterpret_cast<const wchar_t*>(format), args);
	if (length < 0)
		length = 0x200;
	const int written = Write(buffer, length * 2);
	return written < 0 ? written : written >> 1;
}

// --- TMemFileSystem ------------------------------------------------------------------------

TMemFileSystem::TMemFileSystem(const char* name) : TFileSystem(name)
{
	TFileManager* manager = TFileManager::GetFileManager();
	manager->MountFileSystem(this);
	m_pLocalFileSystem = manager->FindFileSystem("local");
}

TMemFileSystem::TMemFileSystem(const TMemFileSystem& other)
    : TFileSystem(other), m_pLocalFileSystem(other.m_pLocalFileSystem)
{
}

TMemFileSystem& TMemFileSystem::operator=(TMemFileSystem& other)
{
	TFileSystem::operator=(other);
	m_pLocalFileSystem = other.m_pLocalFileSystem;
	return *this;
}

TMemFileSystem::~TMemFileSystem()
{
	TFileManager::GetFileManager()->UnmountFileSystem(this);
}

TFile* TMemFileSystem::CreateFile(const TCString& path, unsigned int flags)
{
	auto* file = new TMemFile(this);
	if (file->Open(path, flags))
		return file;
	delete file;
	return nullptr;
}

TFile* TMemFileSystem::CreateFile(TFile* source)
{
	auto* file = new TMemFile(this);
	if (file->Open(source))
		return file;
	delete file;
	return nullptr;
}

void TMemFileSystem::DestroyFile(TFile* file)
{
	if (file == nullptr)
		return;
	auto* memFile = static_cast<TMemFile*>(file);
	memFile->Close();
	delete memFile;
}

TCString TMemFileSystem::MakeInternalPath(const TCString& path) const
{
	return m_pLocalFileSystem->MakeInternalPath(path);
}

} // namespace Toshi
