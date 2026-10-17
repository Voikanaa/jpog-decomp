#pragma once

#include "toshi/kernel/TFile.h"

namespace Toshi
{

class TISOFileSystem;

// Read-only file inside an ISO 9660 image (TKernelInterface.dll). Reads go through an 8 KB
// buffer unless opened with flag 16.
class TISOFile : public TFile
{
	friend class TISOFileSystem;

public:
	TISOFile(const TISOFile& other);
	TISOFile& operator=(const TISOFile& other);

	// Sizes are clamped to the end of the file (negative sizes read to the end).
	int Read(void* buffer, int size) override;
	// Not supported: 0.
	int Write(const void* buffer, int size) override;
	// TSEEK_END sets the position to size - offset; no clamping.
	bool Seek(int offset, TSEEK origin) override;
	int Tell() override;
	int GetSize() override;
	int GetCChar() override;
	// Takes the buffered bytes without checking the end of the file (as the original).
	int GetWChar() override;
	// Not supported: 0.
	int PutCChar(char character) override;
	int PutWChar(unsigned short character) override;
	int VCPrintf(const char* format, char* args) override;
	int VWPrintf(const unsigned short* format, char* args) override;

	TISOFileSystem* GetFileSystem() const;
	// Byte offset of the file in the image and its size.
	void SetSubISOFile(unsigned int offset, unsigned int size);

protected:
	explicit TISOFile(TISOFileSystem* fileSystem);
	~TISOFile() override;
	bool Open(const TCString& path, unsigned int flags);
	void Close();

private:
	int ReadUnbuffered(void* buffer, int size);
	// Loads the 8 KB block at `blockStart` (file offset) into the buffer.
	bool LoadBuffer(int blockStart);

	unsigned int m_uiOffset; // in the image
	unsigned int m_uiSize;
	int m_iPosition;
	unsigned int m_uiFlags; // open flags 16 and 32
	int m_iFilePointer;     // image file position, relative to m_uiOffset
	int m_iBufferStart;     // file offset of the buffer, or -1
	int m_iBufferSize;
	unsigned char* m_pBuffer;
};

JPOG_ASSERT_SIZE(TISOFile, 0x28);

// An ISO 9660 image opened through the file manager; mounts itself on construction. The path
// table is read up front, directory contents on first use. Paths are matched case-insensitively
// and without the ";1" version suffix.
class TISOFileSystem : public TFileSystem
{
	friend class TISOFile;

public:
	class ISO9660Directory;

	// Empty class; its assignment does nothing.
	class ISO9660TimeStamp
	{
	public:
		ISO9660TimeStamp& operator=(const ISO9660TimeStamp& other);
	};

	// Primary volume descriptor (sector 16). Fields are read in place from the image.
	class ISO9660PVD
	{
	public:
		ISO9660PVD();
		ISO9660PVD& operator=(const ISO9660PVD& other);

		void ReadInFromISO(TFile* file);
		unsigned int GetBytePos(unsigned int block) const;
		unsigned int GetPathTableBytePos() const;
		unsigned int GetPathTableSize() const;
		const char* GetVolumeID() const;
		const char* GetVolumeSetID() const;
		// These three return the volume ID (as the original).
		const char* GetCopyrightFileID() const;
		const char* GetAbstractFileID() const;
		const char* GetBibliographicFileID() const;

		static TISOFileSystem* m_pFileSys;

	private:
		char m_szIdentifier[6]; // "CD001"
		char m_szVolumeID[0x20];
		char m_szVolumeSetID[0x80];
		char m_szSystemID[0x20];
		// Both-endian 16-bit pairs at unaligned offsets, kept as read.
		unsigned char m_aVolumeSetSize[4];
		unsigned char m_aVolumeSequenceNumber[4];
		unsigned short m_usBlockSize;
		unsigned short m_usBlockSizeBigEndian;
		unsigned int m_uiPathTableSize;
		unsigned int m_uiPathTableSizeBigEndian;
		unsigned int m_uiPathTableLocation;
		unsigned int m_uiPathTableLocationBigEndian;
	};

	// A file record of a directory.
	class ISO9660File
	{
		friend class ISO9660Directory;

	public:
		ISO9660File& operator=(const ISO9660File& other);

		// Parses a directory record; the name stops at ';'.
		void ReadFromBuffer(unsigned char* record, unsigned char length);
		unsigned int GetLocation() const;
		unsigned int GetLength() const;
		const char* GetName() const;
		// Swaps the records (the last name byte stays, as in the original).
		static void TSTDCALL Swap(ISO9660File& a, ISO9660File& b);

	private:
		unsigned int m_uiLocation;
		unsigned int m_uiLocationBigEndian;
		unsigned int m_uiLength;
		unsigned int m_uiLengthBigEndian;
		char m_szName[0x24];
	};

	// A path table entry; children are contiguous in the table.
	class ISO9660Directory
	{
	public:
		ISO9660Directory();
		~ISO9660Directory();
		ISO9660Directory& operator=(const ISO9660Directory& other);

		// Returns the entry size; `parent` receives the parent's 1-based index (low byte).
		unsigned int ReadInFromPathTable(unsigned char* entry, unsigned char& parent);
		void SetSubDirs(ISO9660Directory* subDirs, unsigned char count);
		ISO9660Directory* GetSubDir(const TCString& name);
		// Binary search over the sorted file list (read on first use).
		ISO9660File* GetFile(const TCString& name);
		// Reads the file records (subdirectories skipped) and sorts them by name.
		void ReadFileList();
		// Debug listing; prints nothing in the shipped build.
		void PrintDirs(bool recursive, bool files, unsigned int indent);
		const char* GetName() const;

		static TISOFileSystem* m_pFileSys;

	private:
		unsigned int m_uiLocation;
		ISO9660Directory* m_pSubDirs;
		unsigned char m_ucSubDirCount;
		ISO9660File* m_pFiles;
		int m_iFileCount; // -1 until read
		char m_szName[0x24];
	};

	// Opens the image at `isoPath` (flags 0x11: read, unbuffered).
	TISOFileSystem(const char* name, const TCString& isoPath);
	TISOFileSystem(const TISOFileSystem& other);
	TISOFileSystem& operator=(TISOFileSystem& other);
	~TISOFileSystem() override;

	// Read-only: needs flag 1.
	TFile* CreateFile(const TCString& path, unsigned int flags) override;
	void DestroyFile(TFile* file) override;
	// Prefix + absolute path, with backslashes in the path part.
	TCString MakeInternalPath(const TCString& path) const override;

	// Points `file` at the image extent of the internal path `path`.
	bool GetSubISOFile(TISOFile* file, const TCString& path);
	TFile* GetISOFile();
	const ISO9660PVD& GetPVD();
	unsigned int GetBytePos(unsigned int block) const;

private:
	ISO9660PVD m_oPVD;
	TFile* m_pISOFile;
	ISO9660Directory* m_pDirectories; // path table order; [0] is the root
	int m_iDirectoryCount;
};

JPOG_ASSERT_SIZE(TISOFileSystem::ISO9660PVD, 0xE4);
JPOG_ASSERT_SIZE(TISOFileSystem::ISO9660File, 0x34);
JPOG_ASSERT_SIZE(TISOFileSystem::ISO9660Directory, 0x38);
JPOG_ASSERT_SIZE(TISOFileSystem, 0x10C);

} // namespace Toshi
