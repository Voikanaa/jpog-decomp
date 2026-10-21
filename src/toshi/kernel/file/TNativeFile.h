#pragma once

#include "toshi/kernel/file/TFile.h"

namespace Toshi
{

class TNativeFileSystem;

// File on the host file system (TKernelInterface.dll; Win32 handles). Reads go through a 2 KB
// sector buffer. Open flags: 1 read, 2 write, 4 read/write, 8 create (truncate), 16 unbuffered.
class TNativeFile : public TFile
{
	friend class TNativeFileSystem;

public:
	TNativeFile(const TNativeFile& other);
	TNativeFile& operator=(const TNativeFile& other);

	int Read(void* buffer, int size) override;
	// Open clears the write-buffered flag in every case (as the original), so writes go straight
	// to the OS.
	int Write(const void* buffer, int size) override;
	bool Seek(int offset, TSEEK origin) override;
	int Tell() override;
	// Moves the OS file pointer to the end (the logical position is kept).
	int GetSize() override;
	// Last write time as a FILETIME value.
	unsigned __int64 GetDate() override;
	int GetCChar() override;
	int GetWChar() override;
	int PutCChar(char character) override;
	int PutWChar(unsigned short character) override;
	// Formats into a 512-character buffer and writes it.
	int VCPrintf(const char* format, char* args) override;
	int VWPrintf(const unsigned short* format, char* args) override;

	TNativeFileSystem* GetFileSystem() const;

protected:
	explicit TNativeFile(TNativeFileSystem* fileSystem);
	~TNativeFile() override;
	bool Open(const TCString& path, unsigned int flags);
	void Close();

private:
	int ReadUnbuffered(void* buffer, int size);
	// Loads the 2 KB sector starting at `sectorStart` into the read buffer.
	bool LoadBuffer(int sectorStart);
	int FlushWriteBuffer();

	void* m_hFile;
	int m_iPosition;    // logical position
	int m_iFilePointer; // OS file pointer
	int m_iBufferStart; // file offset of the read buffer, or -1
	int m_iBufferSize;  // valid bytes in the read buffer
	unsigned char* m_pReadBuffer;
	unsigned char* m_pWriteBuffer;
	int m_iWriteBufferUsed;
	bool m_bWriteBuffered;
};

JPOG_ASSERT_SIZE(TNativeFile, 0x2C);

// The host file system; mounts itself with the file manager on construction.
class TNativeFileSystem : public TFileSystem
{
public:
	explicit TNativeFileSystem(const char* name);
	TNativeFileSystem(const TNativeFileSystem& other);
	TNativeFileSystem& operator=(TNativeFileSystem& other);
	~TNativeFileSystem() override;

	TFile* CreateFile(const TCString& path, unsigned int flags) override;
	void DestroyFile(TFile* file) override;
	// Deletes `path` as given (not made internal), as in the original.
	bool RemoveFile(const TCString& path) override;
	// Prefix + absolute path, with backslashes.
	TCString MakeInternalPath(const TCString& path) const override;
	// Flags: 1 files, 2 directories. An entry of the other kind ends the scan with false; the
	// find handle is not closed when the scan ends (as in the original).
	bool GetFirstFile(const TCString& path, TCString& name, unsigned int flags) override;
	bool GetNextFile(TCString& name, unsigned int flags) override;
	virtual bool MakeDirectory(const TCString& path);

private:
	void* m_hFind;
};

JPOG_ASSERT_SIZE(TNativeFileSystem, 0x20);

} // namespace Toshi
