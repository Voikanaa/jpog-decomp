#pragma once

#include "toshi/kernel/file/TFile.h"

namespace Toshi
{

class TNullFileSystem;

// File that discards writes and has nothing to read (TKernelInterface.dll). It only counts the
// bytes written.
class TNullFile : public TFile
{
	friend class TNullFileSystem;

public:
	TNullFile(const TNullFile& other);
	TNullFile& operator=(const TNullFile& other);

	// Always -1, so GetCChar/GetWChar always fail too.
	int Read(void* buffer, int size) override;
	int Write(const void* buffer, int size) override;
	bool Seek(int offset, TSEEK origin) override;
	int Tell() override;
	int GetSize() override;
	int GetCChar() override;
	int GetWChar() override;
	int PutCChar(char character) override;
	int PutWChar(unsigned short character) override;
	int VCPrintf(const char* format, char* args) override;
	int VWPrintf(const unsigned short* format, char* args) override;

	int GetBytesWritten();

protected:
	explicit TNullFile(TNullFileSystem* fileSystem);
	~TNullFile() override;
	bool Open(const TCString& path, unsigned int flags);
	void Close();

private:
	int m_iUnused; // never set by the original, only copied
	int m_iBytesWritten;
};

JPOG_ASSERT_SIZE(TNullFile, 0x10);

// File system of TNullFiles; mounts itself with the file manager and registers as the single
// instance on construction.
class TNullFileSystem : public TFileSystem
{
public:
	explicit TNullFileSystem(const char* name);
	TNullFileSystem(const TNullFileSystem& other);
	TNullFileSystem& operator=(TNullFileSystem& other);
	~TNullFileSystem() override;

	TFile* CreateFile(const TCString& path, unsigned int flags) override;
	void DestroyFile(TFile* file) override;
	// Always the empty string.
	TCString MakeInternalPath(const TCString& path) const override;

	static TNullFileSystem* TSTDCALL GetNullFileSystem();

private:
	static TNullFileSystem* s_pFileSystem;
};

JPOG_ASSERT_SIZE(TNullFileSystem, 0x1C);

} // namespace Toshi
