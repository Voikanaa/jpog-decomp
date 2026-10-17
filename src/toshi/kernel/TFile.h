#pragma once

#include "common/abi.h"
#include "common/layout.h"
#include "toshi/kernel/TCString.h"
#include "toshi/kernel/TGenericDList.h"
#include "toshi/kernel/TMutex.h"

namespace Toshi
{

class TFileSystem;

// Abstract file (TKernelInterface.dll), created by a TFileSystem. Virtual order matches the
// original vtable (symbols/vtables.csv).
class TFile
{
public:
	enum TSEEK
	{
		TSEEK_SET,
		TSEEK_CUR,
		TSEEK_END
	};

	TFile(const TFile& other);
	TFile& operator=(const TFile& other);

	virtual int Read(void* buffer, int size) = 0;
	virtual int Write(const void* buffer, int size) = 0;
	virtual bool Seek(int offset, TSEEK origin) = 0;
	virtual int Tell() = 0;
	virtual int GetSize() = 0;
	// 0 unless the file system knows the date.
	virtual unsigned __int64 GetDate();
	virtual int GetCChar() = 0;
	virtual int GetWChar() = 0;
	virtual int PutCChar(char character) = 0;
	virtual int PutWChar(unsigned short character) = 0;
	virtual int VCPrintf(const char* format, char* args) = 0;
	virtual int VWPrintf(const unsigned short* format, char* args) = 0;

	int __cdecl CPrintf(const char* format, ...);
	int __cdecl WPrintf(const unsigned short* format, ...);
	TFileSystem* GetFileSystem() const;
	// Returns the file to its file system.
	void Destroy();

	// Opens `path` through the file manager ("fs:path" names a file system).
	static TFile* TSTDCALL Create(const TCString& path, unsigned int flags);
	static void TSTDCALL Destroy(TFile* file);
	// Resolves "." and ".." segments and turns backslashes into slashes.
	static TCString TSTDCALL SimplifyPath(const TCString& path);
	// `path` relative to `base` (absolute paths and drive prefixes win), simplified.
	static TCString TSTDCALL ConcatPath(const TCString& base, const TCString& path);
	// Splits after the last ':', '/' or '\'; the directory keeps its separator.
	static void TSTDCALL SplitPath(const TCString& path, TCString& directory, TCString& file);
	// Debug switch; empty in the shipped build.
	static void TSTDCALL PrintFileAccess(bool enable);

protected:
	explicit TFile(TFileSystem* fileSystem);
	virtual ~TFile();

private:
	TFileSystem* m_pFileSystem;
};

JPOG_ASSERT_SIZE(TFile, 8);

// A named source of files (native disk, ISO, memory, ...), linked into the file manager's lists.
class TFileSystem : public TGenericDList::TNode
{
public:
	explicit TFileSystem(const char* name);
	TFileSystem(const TFileSystem& other);
	// The original takes a non-const reference.
	TFileSystem& operator=(TFileSystem& other);

	virtual ~TFileSystem();
	virtual TFile* CreateFile(const TCString& path, unsigned int flags) = 0;
	virtual void DestroyFile(TFile* file) = 0;
	virtual bool RemoveFile(const TCString& path);
	virtual TCString MakeInternalPath(const TCString& path) const = 0;
	virtual bool GetFirstFile(const TCString& path, TCString& name, unsigned int flags);
	virtual bool GetNextFile(TCString& name, unsigned int flags);
	// Stores the prefix with '/' turned into '\'.
	virtual void SetPrefix(const TCString& prefix);

	const TCString& GetName() const;
	const TCString& GetPrefix() const;

private:
	TCString m_oName;
	TCString m_oPrefix;
};

JPOG_ASSERT_SIZE(TFileSystem, 0x1C);

// Owns the mounted file systems and resolves file names (TKernelInterface.dll). The system path
// (";"-separated file system names) orders the systems searched for plain names.
class TFileManager
{
public:
	class TSysPathIter
	{
	public:
		explicit TSysPathIter(const TCString& path);
		TSysPathIter(const TSysPathIter& other);
		bool First(TCString& name);
		bool Next(TCString& name);

	private:
		const TCString* m_pPath;
		int m_iPosition; // index of the ';' after the current name, or -1
	};

	TFileManager();
	TFileManager(const TFileManager& other);
	~TFileManager();
	TFileManager& operator=(TFileManager& other);

	TFile* CreateFile(const TCString& path, unsigned int flags);
	void DestroyFile(TFile* file);
	void MountFileSystem(TFileSystem* fileSystem);
	void UnmountFileSystem(TFileSystem* fileSystem);
	TFileSystem* FindFileSystem(const TCString& name);
	void SetSystemPath(const TCString& path);
	const TCString& GetSystemPath() const;
	void SetWorkingDirectory(const TCString& path);
	const TCString& GetWorkingDirectory() const;
	TCString MakeAbsolutePath(const TCString& path) const;
	void FileSystemWait();
	void FileSystemRelease();

	static TFileManager* TSTDCALL GetFileManager();

private:
	void ValidateSystemPath();
	void InvalidateSystemPath();
	static TFileSystem* TSTDCALL FindFileSystem(TDList<TFileSystem, 0>& list, const TCString& name);

	static TFileManager* s_pFileManager;

	bool m_bValidated;
	TCString m_oSysPath;
	TCString m_oWorkingDirectory;
	TDList<TFileSystem, 0> m_oValidated; // in system path order
	TDList<TFileSystem, 0> m_oInvalidated;
	TMutex m_oMutex;
};

JPOG_ASSERT_SIZE(TFileManager, 0x28);

} // namespace Toshi
