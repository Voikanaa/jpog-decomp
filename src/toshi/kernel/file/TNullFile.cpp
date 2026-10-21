#include "toshi/kernel/file/TNullFile.h"

#include <cstdio>
#include <cwchar>

namespace Toshi
{

// --- TNullFile -----------------------------------------------------------------------------

TNullFile::TNullFile(TNullFileSystem* fileSystem) : TFile(fileSystem), m_iUnused(0), m_iBytesWritten(0)
{
}

TNullFile::TNullFile(const TNullFile& other)
    : TFile(other), m_iUnused(other.m_iUnused), m_iBytesWritten(other.m_iBytesWritten)
{
}

TNullFile& TNullFile::operator=(const TNullFile& other)
{
	TFile::operator=(other);
	m_iUnused = other.m_iUnused;
	m_iBytesWritten = other.m_iBytesWritten;
	return *this;
}

TNullFile::~TNullFile()
{
}

bool TNullFile::Open(const TCString&, unsigned int)
{
	return true;
}

void TNullFile::Close()
{
}

int TNullFile::GetBytesWritten()
{
	return m_iBytesWritten;
}

int TNullFile::Read(void*, int)
{
	return -1;
}

int TNullFile::Write(const void*, int size)
{
	m_iBytesWritten += size;
	return size;
}

bool TNullFile::Seek(int, TSEEK)
{
	return true;
}

int TNullFile::Tell()
{
	return 0;
}

int TNullFile::GetSize()
{
	return 0;
}

int TNullFile::GetCChar()
{
	unsigned char character = 0;
	if (Read(&character, 1) != 1)
		return -1;
	return character;
}

int TNullFile::GetWChar()
{
	unsigned short character = 0;
	if (Read(&character, 2) != 2)
		return -1;
	return character;
}

int TNullFile::PutCChar(char character)
{
	if (Write(&character, 1) != 1)
		return -1;
	return character;
}

int TNullFile::PutWChar(unsigned short character)
{
	if (Write(&character, 2) != 2)
		return -1;
	return character;
}

int TNullFile::VCPrintf(const char* format, char* args)
{
	char buffer[0x200];
	int length = _vsnprintf(buffer, sizeof(buffer), format, args);
	if (length < 0)
		length = 0x200;
	return Write(buffer, length);
}

int TNullFile::VWPrintf(const unsigned short* format, char* args)
{
	wchar_t buffer[0x200];
	int length = _vsnwprintf(buffer, 0x200, reinterpret_cast<const wchar_t*>(format), args);
	if (length < 0)
		length = 0x200;
	const int written = Write(buffer, length * 2);
	return written < 0 ? written : written >> 1;
}

// --- TNullFileSystem -----------------------------------------------------------------------

TNullFileSystem* TNullFileSystem::s_pFileSystem = nullptr;

TNullFileSystem::TNullFileSystem(const char* name) : TFileSystem(name)
{
	TFileManager::GetFileManager()->MountFileSystem(this);
	s_pFileSystem = this;
}

TNullFileSystem::TNullFileSystem(const TNullFileSystem& other) : TFileSystem(other)
{
}

TNullFileSystem& TNullFileSystem::operator=(TNullFileSystem& other)
{
	TFileSystem::operator=(other);
	return *this;
}

TNullFileSystem::~TNullFileSystem()
{
	TFileManager::GetFileManager()->UnmountFileSystem(this);
	s_pFileSystem = nullptr;
}

TNullFileSystem* TNullFileSystem::GetNullFileSystem()
{
	return s_pFileSystem;
}

TFile* TNullFileSystem::CreateFile(const TCString& path, unsigned int flags)
{
	auto* file = new TNullFile(this);
	if (file->Open(path, flags))
		return file;
	delete file;
	return nullptr;
}

void TNullFileSystem::DestroyFile(TFile* file)
{
	if (file == nullptr)
		return;
	auto* nullFile = static_cast<TNullFile*>(file);
	nullFile->Close();
	delete nullFile;
}

TCString TNullFileSystem::MakeInternalPath(const TCString&) const
{
	return TCString();
}

} // namespace Toshi
