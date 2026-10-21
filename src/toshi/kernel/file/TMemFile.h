#pragma once

#include "toshi/kernel/file/TFile.h"

namespace Toshi
{

class TMemFileSystem;

// Read-only file loaded completely into memory on open (TKernelInterface.dll).
class TMemFile : public TFile
{
	friend class TMemFileSystem;

public:
	TMemFile(const TMemFile& other);
	TMemFile& operator=(const TMemFile& other);

	// All or nothing: -1 unless `size` bytes remain.
	int Read(void* buffer, int size) override;
	// Discards the data and reports success.
	int Write(const void* buffer, int size) override;
	// Positions past the end go back to the start (as the original); negative offsets are not
	// clamped.
	bool Seek(int offset, TSEEK origin) override;
	int Tell() override;
	int GetSize() override;
	// Not supported: -1 and 0xFFFF.
	int GetCChar() override;
	int GetWChar() override;
	int PutCChar(char character) override;
	int PutWChar(unsigned short character) override;
	int VCPrintf(const char* format, char* args) override;
	int VWPrintf(const unsigned short* format, char* args) override;
	// Empty in the shipped build.
	virtual void DebugStats();
	virtual void ClearStats();

	// Seeks to the start; nothing else in the shipped build.
	void DumpFile();

protected:
	explicit TMemFile(TMemFileSystem* fileSystem);
	~TMemFile() override;
	// Loads `path` through the file manager. The source file is never destroyed (its handle
	// stays open) and a missing file still opens as an empty TMemFile, as in the original.
	bool Open(const TCString& path, unsigned int flags);
	// Loads `file` from its start; the caller keeps ownership.
	bool Open(TFile* file);
	void Close();

private:
	unsigned char* m_pData;
	unsigned char* m_pCursor;
	int m_iSize;
};

JPOG_ASSERT_SIZE(TMemFile, 0x14);

// File system of TMemFiles. Paths are resolved by the "local" file system, which must be
// mounted before this one is constructed.
class TMemFileSystem : public TFileSystem
{
public:
	explicit TMemFileSystem(const char* name);
	TMemFileSystem(const TMemFileSystem& other);
	TMemFileSystem& operator=(TMemFileSystem& other);
	~TMemFileSystem() override;

	TFile* CreateFile(const TCString& path, unsigned int flags) override;
	// A memory copy of an open file.
	TFile* CreateFile(TFile* file);
	void DestroyFile(TFile* file) override;
	TCString MakeInternalPath(const TCString& path) const override;

private:
	TFileSystem* m_pLocalFileSystem;
};

JPOG_ASSERT_SIZE(TMemFileSystem, 0x20);

} // namespace Toshi
