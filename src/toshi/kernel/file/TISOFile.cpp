#include "toshi/kernel/file/TISOFile.h"

#include "toshi/kernel/memory/TMemory.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace Toshi
{

namespace
{

constexpr int kBufferSize = 0x2000;
constexpr int kBufferMask = ~(kBufferSize - 1);

// Little-endian fields of image data. "Big-endian" halves are decoded the same way, which keeps
// their in-memory bytes identical to the original's raw copy.
unsigned int U16(const unsigned char* data)
{
	return data[0] | (data[1] << 8);
}

unsigned int U32(const unsigned char* data)
{
	return data[0] | (data[1] << 8) | (data[2] << 16) | (static_cast<unsigned int>(data[3]) << 24);
}

unsigned int ReadU32(TFile* file)
{
	unsigned char data[4] = {};
	file->Read(data, 4);
	return U32(data);
}

} // namespace

// --- TISOFile ------------------------------------------------------------------------------

TISOFile::TISOFile(TISOFileSystem* fileSystem)
    : TFile(fileSystem), m_uiOffset(0), m_uiSize(0), m_iPosition(0), m_iFilePointer(-1), m_iBufferStart(-1),
      m_iBufferSize(0), m_pBuffer(nullptr)
{
}

TISOFile::TISOFile(const TISOFile& other)
    : TFile(other), m_uiOffset(other.m_uiOffset), m_uiSize(other.m_uiSize), m_iPosition(other.m_iPosition),
      m_uiFlags(other.m_uiFlags), m_iFilePointer(other.m_iFilePointer), m_iBufferStart(other.m_iBufferStart),
      m_iBufferSize(other.m_iBufferSize), m_pBuffer(other.m_pBuffer)
{
}

TISOFile& TISOFile::operator=(const TISOFile& other)
{
	TFile::operator=(other);
	m_uiOffset = other.m_uiOffset;
	m_uiSize = other.m_uiSize;
	m_iPosition = other.m_iPosition;
	m_uiFlags = other.m_uiFlags;
	m_iFilePointer = other.m_iFilePointer;
	m_iBufferStart = other.m_iBufferStart;
	m_iBufferSize = other.m_iBufferSize;
	m_pBuffer = other.m_pBuffer;
	return *this;
}

TISOFile::~TISOFile()
{
}

TISOFileSystem* TISOFile::GetFileSystem() const
{
	return static_cast<TISOFileSystem*>(TFile::GetFileSystem());
}

void TISOFile::SetSubISOFile(unsigned int offset, unsigned int size)
{
	m_uiOffset = offset;
	m_uiSize = size;
}

bool TISOFile::Open(const TCString& path, unsigned int flags)
{
	TISOFileSystem* fileSystem = GetFileSystem();
	const TCString internalPath = fileSystem->MakeInternalPath(path);
	m_uiFlags = flags & 0x30;
	if (!fileSystem->GetSubISOFile(this, internalPath))
		return false;
	if ((m_uiFlags & 0x10) == 0)
		m_pBuffer = static_cast<unsigned char*>(tmemalign(0x40, kBufferSize));
	return true;
}

void TISOFile::Close()
{
	m_uiOffset = 0;
	m_uiSize = 0;
	m_iPosition = 0;
	m_iFilePointer = -1;
	m_iBufferStart = -1;
	m_iBufferSize = 0;
	if (m_pBuffer != nullptr)
	{
		tfree(m_pBuffer);
		m_pBuffer = nullptr;
	}
}

int TISOFile::ReadUnbuffered(void* buffer, int size)
{
	if (m_uiSize < static_cast<unsigned int>(size + m_iPosition))
		size = static_cast<int>(m_uiSize) - m_iPosition;
	TFile* image = GetFileSystem()->m_pISOFile;
	if (m_iPosition != m_iFilePointer)
	{
		if (!image->Seek(static_cast<int>(m_uiOffset) + m_iPosition, TSEEK_SET))
			return 0;
		m_iFilePointer = m_iPosition;
	}
	if (image->Tell() != m_iPosition + static_cast<int>(m_uiOffset))
		image->Seek(m_iPosition + static_cast<int>(m_uiOffset), TSEEK_SET);
	const int read = image->Read(buffer, size);
	if (read < 0)
		return 0;
	m_iFilePointer += read;
	m_iPosition = m_iFilePointer;
	return read;
}

bool TISOFile::LoadBuffer(int blockStart)
{
	if (blockStart == m_iBufferStart)
		return true;
	TFile* image = GetFileSystem()->m_pISOFile;
	if (blockStart != m_iFilePointer)
	{
		if (!image->Seek(static_cast<int>(m_uiOffset) + blockStart, TSEEK_SET))
			return false;
		m_iFilePointer = blockStart;
	}
	if (image->Tell() != static_cast<int>(m_uiOffset) + blockStart)
		image->Seek(static_cast<int>(m_uiOffset) + blockStart, TSEEK_SET);
	const int read = image->Read(m_pBuffer, kBufferSize);
	if (read < 0)
		return false;
	m_iFilePointer += read;
	m_iBufferSize = read;
	m_iBufferStart = blockStart;
	return true;
}

// Same scheme as TNativeFile::Read with an 8 KB block: bytes before the end position's block
// come from the buffer (if it holds the current block) and direct reads, the tail from the
// buffered end block.
int TISOFile::Read(void* buffer, int size)
{
	const auto position = static_cast<unsigned int>(m_iPosition);
	if (m_uiSize <= position)
		return 0;
	if (m_uiSize < static_cast<unsigned int>(size) + position)
		size = static_cast<int>(m_uiSize - position);
	if (m_pBuffer == nullptr)
		return ReadUnbuffered(buffer, size);

	auto* out = static_cast<unsigned char*>(buffer);
	int total = 0;
	int block = m_iPosition & kBufferMask;
	const int endBlock = (m_iPosition + size) & kBufferMask;
	if (block != endBlock)
	{
		if (block == m_iBufferStart)
		{
			const int available = m_iBufferSize - (m_iPosition - block);
			if (available > 0)
			{
				std::memcpy(out, m_pBuffer + (m_iPosition - block), static_cast<std::size_t>(available));
				out += available;
				m_iPosition += available;
				total = available;
			}
		}
		const int direct = endBlock - m_iPosition;
		block = endBlock;
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
		if (!LoadBuffer(block))
			return total;
		int count = size - total;
		const int available = m_iBufferSize - (m_iPosition - block);
		if (available < count)
			count = available;
		if (count > 0)
		{
			std::memcpy(out, m_pBuffer + (m_iPosition - block), static_cast<std::size_t>(count));
			m_iPosition += count;
			total += count;
		}
	}
	return total;
}

int TISOFile::Write(const void*, int)
{
	return 0;
}

bool TISOFile::Seek(int offset, TSEEK origin)
{
	switch (origin)
	{
	case TSEEK_SET:
		m_iPosition = offset;
		break;
	case TSEEK_CUR:
		m_iPosition += offset;
		break;
	case TSEEK_END:
		m_iPosition = static_cast<int>(m_uiSize) - offset;
		break;
	}
	return true;
}

int TISOFile::Tell()
{
	return m_iPosition;
}

int TISOFile::GetSize()
{
	return static_cast<int>(m_uiSize);
}

int TISOFile::GetCChar()
{
	unsigned char character = 0;
	if (Read(&character, 1) != 1)
		return -1;
	return character;
}

int TISOFile::GetWChar()
{
	if (m_pBuffer != nullptr)
	{
		const int block = m_iPosition & kBufferMask;
		const int offset = m_iPosition - block;
		if (block == m_iBufferStart && offset <= m_iBufferSize - 2)
		{
			m_iPosition += 2;
			return m_pBuffer[offset] | (m_pBuffer[offset + 1] << 8);
		}
	}
	unsigned short character = 0;
	if (Read(&character, 2) != 2)
		return -1;
	return character;
}

int TISOFile::PutCChar(char)
{
	return 0;
}

int TISOFile::PutWChar(unsigned short)
{
	return 0;
}

int TISOFile::VCPrintf(const char*, char*)
{
	return 0;
}

int TISOFile::VWPrintf(const unsigned short*, char*)
{
	return 0;
}

// --- ISO9660TimeStamp / ISO9660PVD ---------------------------------------------------------

TISOFileSystem* TISOFileSystem::ISO9660PVD::m_pFileSys = nullptr;
TISOFileSystem* TISOFileSystem::ISO9660Directory::m_pFileSys = nullptr;

TISOFileSystem::ISO9660TimeStamp& TISOFileSystem::ISO9660TimeStamp::operator=(const ISO9660TimeStamp&)
{
	return *this;
}

TISOFileSystem::ISO9660PVD::ISO9660PVD()
{
}

TISOFileSystem::ISO9660PVD& TISOFileSystem::ISO9660PVD::operator=(const ISO9660PVD& other) = default;

// Reads the descriptor field by field, skipping what the engine does not keep, and leaves the
// file positioned after the descriptor sector.
void TISOFileSystem::ISO9660PVD::ReadInFromISO(TFile* file)
{
	unsigned char scratch[8] = {};
	file->Seek(0x8000, TFile::TSEEK_SET);
	file->Read(scratch, 1); // descriptor type
	file->Read(m_szIdentifier, 5);
	m_szIdentifier[5] = '\0';
	file->Read(scratch, 1); // version
	file->Seek(1, TFile::TSEEK_CUR);
	file->Read(m_szSystemID, 0x20);
	file->Read(m_szVolumeID, 0x20);
	file->Seek(8, TFile::TSEEK_CUR);
	file->Read(scratch, 8); // volume space size (the original reads it over the next two fields)
	file->Seek(0x20, TFile::TSEEK_CUR);
	file->Read(m_aVolumeSetSize, 4);
	file->Read(m_aVolumeSequenceNumber, 4);
	unsigned char pair[8] = {};
	file->Read(pair, 4);
	m_usBlockSize = static_cast<unsigned short>(U16(pair));
	m_usBlockSizeBigEndian = static_cast<unsigned short>(U16(pair + 2));
	file->Read(pair, 8);
	m_uiPathTableSize = U32(pair);
	m_uiPathTableSizeBigEndian = U32(pair + 4);
	m_uiPathTableLocation = ReadU32(file);
	file->Read(scratch, 4); // optional path table
	m_uiPathTableLocationBigEndian = ReadU32(file);
	file->Read(scratch, 4);
	file->Seek(0x22, TFile::TSEEK_CUR); // root directory record
	file->Read(m_szVolumeSetID, 0x80);
	static const int skipped[] = {0x80, 0x80, 0x80, 0x25, 0x25, 0x25, 0x11, 0x11, 0x11, 0x11, 1, 1, 0x200, 0x28C};
	for (const int count : skipped)
		file->Seek(count, TFile::TSEEK_CUR);
}

unsigned int TISOFileSystem::ISO9660PVD::GetBytePos(unsigned int block) const
{
	return m_usBlockSize * block;
}

// Only the low 16 bits of the location are used (as the original).
unsigned int TISOFileSystem::ISO9660PVD::GetPathTableBytePos() const
{
	return static_cast<unsigned int>(m_usBlockSize) * (m_uiPathTableLocation & 0xFFFF);
}

unsigned int TISOFileSystem::ISO9660PVD::GetPathTableSize() const
{
	return m_uiPathTableSize;
}

const char* TISOFileSystem::ISO9660PVD::GetVolumeID() const
{
	return m_szVolumeID;
}

const char* TISOFileSystem::ISO9660PVD::GetVolumeSetID() const
{
	return m_szVolumeSetID;
}

const char* TISOFileSystem::ISO9660PVD::GetCopyrightFileID() const
{
	return m_szVolumeID;
}

const char* TISOFileSystem::ISO9660PVD::GetAbstractFileID() const
{
	return m_szVolumeID;
}

const char* TISOFileSystem::ISO9660PVD::GetBibliographicFileID() const
{
	return m_szVolumeID;
}

// --- ISO9660File ---------------------------------------------------------------------------

TISOFileSystem::ISO9660File& TISOFileSystem::ISO9660File::operator=(const ISO9660File& other) = default;

// The name copy is clamped to the name buffer (the original copies the full record length).
void TISOFileSystem::ISO9660File::ReadFromBuffer(unsigned char* record, unsigned char)
{
	m_uiLocation = U32(record + 2);
	m_uiLocationBigEndian = U32(record + 6);
	m_uiLength = U32(record + 10);
	m_uiLengthBigEndian = U32(record + 14);
	const unsigned int length = record[0x20];
	std::memcpy(m_szName, record + 0x21, std::min<unsigned int>(length, sizeof(m_szName)));
	for (unsigned int i = 0; i < length && i < sizeof(m_szName); ++i)
		if (m_szName[i] == ';')
		{
			m_szName[i] = '\0';
			return;
		}
}

unsigned int TISOFileSystem::ISO9660File::GetLocation() const
{
	return m_uiLocation;
}

unsigned int TISOFileSystem::ISO9660File::GetLength() const
{
	return m_uiLength;
}

const char* TISOFileSystem::ISO9660File::GetName() const
{
	return m_szName;
}

void TISOFileSystem::ISO9660File::Swap(ISO9660File& a, ISO9660File& b)
{
	std::swap(a.m_uiLocation, b.m_uiLocation);
	std::swap(a.m_uiLocationBigEndian, b.m_uiLocationBigEndian);
	std::swap(a.m_uiLength, b.m_uiLength);
	std::swap(a.m_uiLengthBigEndian, b.m_uiLengthBigEndian);
	std::swap_ranges(a.m_szName, a.m_szName + sizeof(a.m_szName) - 1, b.m_szName);
}

// --- ISO9660Directory ----------------------------------------------------------------------

TISOFileSystem::ISO9660Directory::ISO9660Directory() : m_pSubDirs(nullptr), m_pFiles(nullptr), m_iFileCount(-1)
{
}

TISOFileSystem::ISO9660Directory::~ISO9660Directory()
{
	tfree(m_pFiles);
}

TISOFileSystem::ISO9660Directory& TISOFileSystem::ISO9660Directory::operator=(const ISO9660Directory& other) = default;

// The name copy is clamped to the name buffer (the original copies the full entry length).
unsigned int TISOFileSystem::ISO9660Directory::ReadInFromPathTable(unsigned char* entry, unsigned char& parent)
{
	const unsigned int length = entry[0];
	m_uiLocation = U32(entry + 2);
	parent = static_cast<unsigned char>(U16(entry + 6));
	const unsigned int copied = std::min<unsigned int>(length, sizeof(m_szName) - 1);
	std::memcpy(m_szName, entry + 8, copied);
	m_szName[copied] = '\0';
	return length + 8 + (length & 1);
}

void TISOFileSystem::ISO9660Directory::SetSubDirs(ISO9660Directory* subDirs, unsigned char count)
{
	m_pSubDirs = subDirs;
	m_ucSubDirCount = count;
}

TISOFileSystem::ISO9660Directory* TISOFileSystem::ISO9660Directory::GetSubDir(const TCString& name)
{
	for (int i = 0; i < m_ucSubDirCount; ++i)
		if (name.CompareNoCase(m_pSubDirs[i].m_szName, -1) == 0)
			return &m_pSubDirs[i];
	return nullptr;
}

TISOFileSystem::ISO9660File* TISOFileSystem::ISO9660Directory::GetFile(const TCString& name)
{
	if (m_iFileCount < 0)
		ReadFileList();
	int high = m_iFileCount;
	int low = 0;
	int middle = high >> 1;
	if (middle < 0)
		return nullptr;
	do
	{
		if (high <= middle)
			return nullptr;
		const int compare = name.CompareNoCase(m_pFiles[middle].m_szName, -1);
		if (compare == 0)
			return &m_pFiles[middle];
		if (compare >= 0)
			low = middle + 1;
		else
			high = middle;
		middle = ((high - low) >> 1) + low;
	} while (low <= middle);
	return nullptr;
}

void TISOFileSystem::ISO9660Directory::ReadFileList()
{
	TISOFileSystem* fileSystem = m_pFileSys;
	m_iFileCount = 0;
	TFile* image = fileSystem->m_pISOFile;
	// The directory's own "." record gives its extent and size.
	image->Seek(static_cast<int>(fileSystem->GetBytePos(m_uiLocation & 0xFFFF)), TFile::TSEEK_SET);
	unsigned char self[0x20] = {};
	image->Read(self, 0x20);
	const unsigned int extent = U32(self + 2);
	const unsigned int size = U32(self + 10);
	auto* data = static_cast<unsigned char*>(tmalloc(static_cast<int>(size), nullptr, -1));
	image->Seek(static_cast<int>(fileSystem->GetBytePos(extent & 0xFFFF)), TFile::TSEEK_SET);
	image->Read(data, static_cast<int>(size));

	// Records never cross sectors: zero bytes pad to the next one. Subdirectories are skipped.
	int count = 0;
	const unsigned char* record = data;
	for (unsigned int remaining = size; remaining > 0;)
	{
		const unsigned int length = record[0];
		if (length == 0)
		{
			--remaining;
			++record;
			continue;
		}
		if ((record[0x19] & 2) == 0)
			++count;
		remaining -= length;
		record += length;
	}
	m_iFileCount = count;
	m_pFiles = static_cast<ISO9660File*>(tmalloc(count * static_cast<int>(sizeof(ISO9660File)), nullptr, -1));
	int index = 0;
	unsigned char* next = data;
	for (unsigned int remaining = size; remaining > 0;)
	{
		const unsigned int length = next[0];
		if (length == 0)
		{
			--remaining;
			++next;
			continue;
		}
		remaining -= length;
		if ((next[0x19] & 2) == 0)
			m_pFiles[index++].ReadFromBuffer(next, next[0]);
		next += length;
	}

	// Selection sort by name, as the original.
	TCString smallest;
	for (int i = 0; i < count - 1; ++i)
	{
		smallest = m_pFiles[i].m_szName;
		for (int j = i + 1; j < count; ++j)
			if (smallest.CompareNoCase(m_pFiles[j].m_szName, -1) > 0)
			{
				ISO9660File::Swap(m_pFiles[i], m_pFiles[j]);
				smallest = m_pFiles[i].m_szName;
			}
	}
	tfree(data);
}

void TISOFileSystem::ISO9660Directory::PrintDirs(bool recursive, bool files, unsigned int indent)
{
	if (files && m_iFileCount < 0)
		ReadFileList();
	if (m_ucSubDirCount == 0 || !recursive)
		return;
	for (int i = 0; i < m_ucSubDirCount; ++i)
		m_pSubDirs[i].PrintDirs(true, files, indent + 4);
}

const char* TISOFileSystem::ISO9660Directory::GetName() const
{
	return m_szName;
}

// --- TISOFileSystem ------------------------------------------------------------------------

TISOFileSystem::TISOFileSystem(const char* name, const TCString& isoPath)
    : TFileSystem(name), m_pISOFile(nullptr), m_pDirectories(nullptr), m_iDirectoryCount(0)
{
	ISO9660PVD::m_pFileSys = this;
	ISO9660Directory::m_pFileSys = this;
	TFileManager::GetFileManager()->MountFileSystem(this);
	m_pISOFile = TFile::Create(isoPath, 0x11);
	if (m_pISOFile == nullptr)
		return;
	m_oPVD.ReadInFromISO(m_pISOFile);

	const unsigned int tableSize = m_oPVD.GetPathTableSize();
	auto* table = static_cast<unsigned char*>(tmalloc(static_cast<int>(tableSize), nullptr, -1));
	m_pISOFile->Seek(static_cast<int>(m_oPVD.GetPathTableBytePos()), TFile::TSEEK_SET);
	m_pISOFile->Read(table, static_cast<int>(tableSize));
	int remaining = static_cast<int>(tableSize);
	const unsigned char* entry = table;
	while (remaining != 0)
	{
		const int length = entry[0] + 8 + (entry[0] & 1);
		remaining -= length;
		entry += length;
		++m_iDirectoryCount;
	}

	m_pDirectories = new ISO9660Directory[static_cast<std::size_t>(m_iDirectoryCount)];
	std::vector<unsigned char> parents(static_cast<std::size_t>(m_iDirectoryCount));
	unsigned char* next = table;
	for (int i = 0; i < m_iDirectoryCount; ++i)
		next += m_pDirectories[i].ReadInFromPathTable(next, parents[static_cast<std::size_t>(i)]);
	// A directory's children are the entries naming it as parent; the table lists them together.
	for (int i = 0; i < m_iDirectoryCount; ++i)
	{
		ISO9660Directory* first = nullptr;
		unsigned char count = 0;
		for (int j = 0; j < m_iDirectoryCount; ++j)
			if (parents[static_cast<std::size_t>(j)] - 1 == i && j != i)
			{
				++count;
				if (first == nullptr)
					first = &m_pDirectories[j];
			}
		m_pDirectories[i].SetSubDirs(first, count);
	}
	tfree(table);
}

TISOFileSystem::TISOFileSystem(const TISOFileSystem& other)
    : TFileSystem(other), m_oPVD(other.m_oPVD), m_pISOFile(other.m_pISOFile), m_pDirectories(other.m_pDirectories),
      m_iDirectoryCount(other.m_iDirectoryCount)
{
}

TISOFileSystem& TISOFileSystem::operator=(TISOFileSystem& other)
{
	TFileSystem::operator=(other);
	m_oPVD = other.m_oPVD;
	m_pISOFile = other.m_pISOFile;
	m_pDirectories = other.m_pDirectories;
	m_iDirectoryCount = other.m_iDirectoryCount;
	return *this;
}

TISOFileSystem::~TISOFileSystem()
{
	if (m_pISOFile != nullptr)
		TFile::Destroy(m_pISOFile);
	TFileManager::GetFileManager()->UnmountFileSystem(this);
	delete[] m_pDirectories;
}

TFile* TISOFileSystem::CreateFile(const TCString& path, unsigned int flags)
{
	if ((flags & 1) == 0)
		return nullptr;
	auto* file = new TISOFile(this);
	if (file->Open(path, flags))
		return file;
	delete file;
	return nullptr;
}

void TISOFileSystem::DestroyFile(TFile* file)
{
	if (file == nullptr)
		return;
	auto* isoFile = static_cast<TISOFile*>(file);
	isoFile->Close();
	delete isoFile;
}

TCString TISOFileSystem::MakeInternalPath(const TCString& path) const
{
	TCString result(GetPrefix());
	const int length = result.Length();
	if (length > 0 && (result[length - 1] == '/' || result[length - 1] == '\\'))
		result.Truncate(length - 1);
	TCString absolute(TFileManager::GetFileManager()->MakeAbsolutePath(path));
	for (int i = 0; i < absolute.Length(); ++i)
		if (absolute[i] == '/')
			absolute[i] = '\\';
	result += absolute;
	return result;
}

// Splits the path at its backslashes: every segment but the last names a subdirectory, starting
// at the root, and the last names the file.
bool TISOFileSystem::GetSubISOFile(TISOFile* file, const TCString& path)
{
	std::vector<int> separators;
	for (int i = 0; i < path.Length(); ++i)
		if (path[i] == '\\')
			separators.push_back(i);
	const int count = static_cast<int>(separators.size());
	const int fileStart = count < 2 ? 1 : separators[static_cast<std::size_t>(count - 1)] + 1;
	const TCString fileName(path.GetString(fileStart));

	ISO9660Directory* directory = m_pDirectories;
	TCString segment;
	for (int i = 0; directory != nullptr; ++i)
	{
		if (i >= count - 1)
		{
			const ISO9660File* entry = directory->GetFile(fileName);
			if (entry == nullptr)
				return false;
			file->SetSubISOFile(GetBytePos(entry->GetLocation()), entry->GetLength());
			return true;
		}
		const int start = separators[static_cast<std::size_t>(i)];
		segment.Copy(TCString(path.GetString(start + 1)), separators[static_cast<std::size_t>(i + 1)] - start - 1);
		directory = directory->GetSubDir(segment);
	}
	return false;
}

TFile* TISOFileSystem::GetISOFile()
{
	return m_pISOFile;
}

const TISOFileSystem::ISO9660PVD& TISOFileSystem::GetPVD()
{
	return m_oPVD;
}

unsigned int TISOFileSystem::GetBytePos(unsigned int block) const
{
	return m_oPVD.GetBytePos(block);
}

} // namespace Toshi
