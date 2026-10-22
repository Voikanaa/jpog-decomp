#include "toshi/kernel/file/TFile.h"

#include <cstdarg>
#include <cstring>

namespace Toshi
{

TFileManager* TFileManager::s_pFileManager = nullptr;

// --- TFile ---------------------------------------------------------------------------------

TFile::TFile(TFileSystem* fileSystem) : m_pFileSystem(fileSystem)
{
}

TFile::TFile(const TFile& other) : m_pFileSystem(other.m_pFileSystem)
{
}

TFile::~TFile()
{
}

TFile& TFile::operator=(const TFile& other)
{
	m_pFileSystem = other.m_pFileSystem;
	return *this;
}

unsigned long long TFile::GetDate()
{
	return 0;
}

int TFile::CPrintf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	const int result = VCPrintf(format, args);
	va_end(args);
	return result;
}

int TFile::WPrintf(const unsigned short* format, ...)
{
	va_list args;
	va_start(args, format);
	const int result = VWPrintf(format, args);
	va_end(args);
	return result;
}

TFileSystem* TFile::GetFileSystem() const
{
	return m_pFileSystem;
}

void TFile::Destroy()
{
	m_pFileSystem->DestroyFile(this);
}

TFile* TSTDCALL TFile::Create(const TCString& path, unsigned int flags)
{
	return TFileManager::GetFileManager()->CreateFile(path, flags);
}

void TSTDCALL TFile::Destroy(TFile* file)
{
	if (file != nullptr)
		file->m_pFileSystem->DestroyFile(file);
}

void TSTDCALL TFile::PrintFileAccess(bool)
{
}

TCString TSTDCALL TFile::SimplifyPath(const TCString& path)
{
	TCString source(path);
	TCString result;
	for (int i = 0; i < source.Length(); ++i)
		if (source[i] == '\\')
			source[i] = '/';

	int start = 0;
	const int drive = source.FindReverse(':', -1);
	if (drive >= 0)
	{
		start = drive + 1;
		result.Concat(source, start);
	}
	if (source[start] == '/')
	{
		++start;
		result += "/";
	}
	// `root` is where ".." can no longer climb (it grows by leading "../" segments); trailing
	// slashes are trimmed only beyond the original root.
	int root = result.Length();
	const int initialRoot = root;
	source += "/";
	for (int slash = source.Find('/', start); slash >= 0; slash = source.Find('/', start))
	{
		const char* segment = source.GetString(start);
		if (std::strncmp(segment, "../", 3) == 0)
		{
			const int length = result.Length();
			if (root == length)
			{
				result += "../";
				root += 3;
			}
			else if (length < 2)
			{
				result.Truncate(root);
			}
			else
			{
				const int previous = result.FindReverse('/', length - 2);
				result.Truncate(previous < root ? root : previous + 1);
			}
		}
		else if (std::strncmp(segment, "./", 2) != 0)
		{
			result.Concat(segment, slash - start + 1);
		}
		start = slash + 1;
	}
	if (initialRoot < result.Length())
		result.Truncate(result.Length() - 1);
	return result;
}

TCString TSTDCALL TFile::ConcatPath(const TCString& base, const TCString& path)
{
	TCString result;
	int baseStart = 0;
	int pathStart = 0;
	const int pathDrive = path.FindReverse(':', -1);
	if (pathDrive < 0)
	{
		const int baseDrive = base.FindReverse(':', -1);
		if (baseDrive >= 0)
		{
			baseStart = baseDrive + 1;
			result.Concat(base, baseStart);
		}
	}
	else
	{
		pathStart = pathDrive + 1;
		result.Concat(path, pathStart);
		baseStart = base.FindReverse(':', -1) + 1;
	}

	const int baseLength = base.Length();
	const int pathLength = path.Length();
	if (pathStart < pathLength)
	{
		const char* rest = path.GetString(pathStart);
		if (*rest == '/' || *rest == '\\')
		{
			result.Concat(rest, -1);
			return SimplifyPath(result);
		}
	}
	result.Concat(base.GetString(baseStart), -1);
	if (baseStart < baseLength && pathStart < pathLength)
	{
		const char last = *base.GetString(baseLength - 1);
		if (last != '/' && last != '\\')
			result += "/";
	}
	result.Concat(path.GetString(pathStart), -1);
	return SimplifyPath(result);
}

void TSTDCALL TFile::SplitPath(const TCString& path, TCString& directory, TCString& file)
{
	for (int i = path.Length() - 1; i >= 0; --i)
	{
		const char* separator = path.GetString(i);
		if (*separator == ':' || *separator == '/' || *separator == '\\')
		{
			directory.Copy(path, i + 1);
			file.Copy(separator + 1, -1);
			return;
		}
	}
	directory = "";
	file = static_cast<const char*>(path);
}

// --- TFileSystem ---------------------------------------------------------------------------

TFileSystem::TFileSystem(const char* name) : m_oName(name)
{
}

TFileSystem::TFileSystem(const TFileSystem& other)
    : TGenericDList::TNode(other), m_oName(other.m_oName), m_oPrefix(other.m_oPrefix)
{
}

TFileSystem& TFileSystem::operator=(TFileSystem& other)
{
	TGenericDList::TNode::operator=(other);
	m_oName = other.m_oName;
	m_oPrefix = other.m_oPrefix;
	return *this;
}

TFileSystem::~TFileSystem()
{
}

bool TFileSystem::RemoveFile(const TCString&)
{
	return true;
}

bool TFileSystem::GetFirstFile(const TCString&, TCString&, unsigned int)
{
	return false;
}

bool TFileSystem::GetNextFile(TCString&, unsigned int)
{
	return false;
}

void TFileSystem::SetPrefix(const TCString& prefix)
{
	m_oPrefix = static_cast<const char*>(prefix);
	for (int i = 0; i < m_oPrefix.Length(); ++i)
		if (m_oPrefix[i] == '/')
			m_oPrefix[i] = '\\';
}

const TCString& TFileSystem::GetName() const
{
	return m_oName;
}

const TCString& TFileSystem::GetPrefix() const
{
	return m_oPrefix;
}

// --- TFileManager --------------------------------------------------------------------------

TFileManager::TSysPathIter::TSysPathIter(const TCString& path) : m_pPath(&path), m_iPosition(-1)
{
}

TFileManager::TSysPathIter::TSysPathIter(const TSysPathIter& other)
    : m_pPath(other.m_pPath), m_iPosition(other.m_iPosition)
{
}

bool TFileManager::TSysPathIter::First(TCString& name)
{
	if (m_pPath->Length() <= 0)
	{
		m_iPosition = -1;
		return false;
	}
	m_iPosition = m_pPath->Find(';', 0);
	name.Copy(*m_pPath, m_iPosition < 0 ? -1 : m_iPosition);
	return true;
}

bool TFileManager::TSysPathIter::Next(TCString& name)
{
	if (m_iPosition < 0)
		return false;
	const int start = m_iPosition + 1;
	m_iPosition = m_pPath->Find(';', start);
	name.Copy(m_pPath->GetString(start), m_iPosition < 0 ? -1 : m_iPosition - start);
	return true;
}

TFileManager::TFileManager() : m_bValidated(false), m_oWorkingDirectory("/")
{
	s_pFileManager = this;
	m_oMutex.Create();
}

// A raw copy, including the list sentinels' links, as in the original.
TFileManager::TFileManager(const TFileManager& other)
    : m_bValidated(other.m_bValidated), m_oSysPath(other.m_oSysPath), m_oWorkingDirectory(other.m_oWorkingDirectory),
      m_oValidated(), m_oInvalidated(), m_oMutex(other.m_oMutex)
{
	static_cast<TGenericDList&>(m_oValidated) = other.m_oValidated;
	static_cast<TGenericDList&>(m_oInvalidated) = other.m_oInvalidated;
}

TFileManager::~TFileManager()
{
	s_pFileManager = nullptr;
	m_oMutex.Destroy();
	m_oInvalidated.RemoveAll();
	m_oValidated.RemoveAll();
}

TFileManager& TFileManager::operator=(TFileManager& other)
{
	m_bValidated = other.m_bValidated;
	m_oSysPath = other.m_oSysPath;
	m_oWorkingDirectory = other.m_oWorkingDirectory;
	static_cast<TGenericDList&>(m_oValidated) = other.m_oValidated;
	static_cast<TGenericDList&>(m_oInvalidated) = other.m_oInvalidated;
	m_oMutex = other.m_oMutex;
	return *this;
}

TFile* TFileManager::CreateFile(const TCString& path, unsigned int flags)
{
	ValidateSystemPath();
	const int colon = path.Find(':', 0);
	if (colon >= 0)
	{
		TCString systemName;
		TCString rest;
		systemName.Copy(path, colon);
		rest.Copy(path.GetString(colon + 1), -1);
		if (TFileSystem* fileSystem = FindFileSystem(systemName))
			return fileSystem->CreateFile(rest, flags);
	}
	for (TFileSystem* fileSystem = m_oValidated.Head(); fileSystem; fileSystem = m_oValidated.Next(fileSystem))
	{
		if (TFile* file = fileSystem->CreateFile(path, flags))
			return file;
	}
	return nullptr;
}

void TFileManager::DestroyFile(TFile* file)
{
	if (file != nullptr)
		file->GetFileSystem()->DestroyFile(file);
}

void TFileManager::MountFileSystem(TFileSystem* fileSystem)
{
	m_oInvalidated.InsertTail(fileSystem);
	InvalidateSystemPath();
}

void TFileManager::UnmountFileSystem(TFileSystem* fileSystem)
{
	TDList<TFileSystem, 0>::Remove(fileSystem);
}

TFileSystem* TFileManager::FindFileSystem(const TCString& name)
{
	if (TFileSystem* fileSystem = FindFileSystem(m_oValidated, name))
		return fileSystem;
	return FindFileSystem(m_oInvalidated, name);
}

TFileSystem* TSTDCALL TFileManager::FindFileSystem(TDList<TFileSystem, 0>& list, const TCString& name)
{
	for (TFileSystem* fileSystem = list.Head(); fileSystem; fileSystem = list.Next(fileSystem))
		if (name == fileSystem->GetName())
			return fileSystem;
	return nullptr;
}

void TFileManager::SetSystemPath(const TCString& path)
{
	m_oSysPath = static_cast<const char*>(path);
	InvalidateSystemPath();
}

const TCString& TFileManager::GetSystemPath() const
{
	return m_oSysPath;
}

void TFileManager::SetWorkingDirectory(const TCString& path)
{
	TCString absolute = MakeAbsolutePath(path);
	m_oWorkingDirectory = absolute;
}

const TCString& TFileManager::GetWorkingDirectory() const
{
	return m_oWorkingDirectory;
}

TCString TFileManager::MakeAbsolutePath(const TCString& path) const
{
	return TFile::ConcatPath(m_oWorkingDirectory, path);
}

void TFileManager::FileSystemWait()
{
	m_oMutex.Lock(0);
}

void TFileManager::FileSystemRelease()
{
	m_oMutex.Unlock();
}

TFileManager* TSTDCALL TFileManager::GetFileManager()
{
	return s_pFileManager;
}

// Moves every validated file system back to the head of the other list (tail first), then pulls
// the systems named in the system path back, in path order.
void TFileManager::ValidateSystemPath()
{
	if (m_bValidated)
		return;
	while (!m_oValidated.IsEmpty())
	{
		TFileSystem* fileSystem = m_oValidated.Tail();
		TDList<TFileSystem, 0>::Remove(fileSystem);
		m_oInvalidated.InsertHead(fileSystem);
	}
	TSysPathIter iter(m_oSysPath);
	TCString name;
	for (bool more = iter.First(name); more; more = iter.Next(name))
	{
		if (TFileSystem* fileSystem = FindFileSystem(m_oInvalidated, name))
		{
			TDList<TFileSystem, 0>::Remove(fileSystem);
			m_oValidated.InsertTail(fileSystem);
		}
	}
	m_bValidated = true;
}

void TFileManager::InvalidateSystemPath()
{
	m_bValidated = false;
}

} // namespace Toshi
