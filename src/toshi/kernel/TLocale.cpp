#include "toshi/kernel/TLocale.h"

#include "toshi/kernel/TSystem.h"
#include "toshi/kernel/file/TFile.h"
#include "toshi/kernel/file/TFileLexer.h"

#include <cstdarg>
#include <intrin.h>
#include <type_traits>

namespace Toshi
{

const int TLocale::TEMP_START_INDEX = 0x40000000;
TFreeList TLocalisedString::ms_oFreelist(sizeof(TLocalisedString), 0, 8, const_cast<char*>("TLocalisedString"));
const TCString TLString::ms_csEmpty;
const TWString TLString::ms_wsEmpty;

// --- TLocaleInfo ---------------------------------------------------------------------------

TLocaleInfo::TLocaleInfo(int index, const TPCString& name, const TPCString& fileName)
    : m_iIndex(index), m_oName(name), m_oFileName(fileName), m_eWordBreakStyle(WORDBREAK_SPACES)
{
	if (name.GetCString().Compare("Japanese", -1) == 0)
		m_eWordBreakStyle = WORDBREAK_ANYWHERE;
}

TLocaleInfo::TLocaleInfo(const TLocaleInfo& other)
    : m_iIndex(other.m_iIndex), m_oName(other.m_oName), m_oFileName(other.m_oFileName),
      m_eWordBreakStyle(other.m_eWordBreakStyle)
{
}

TLocaleInfo::~TLocaleInfo()
{
}

TLocaleInfo& TLocaleInfo::operator=(const TLocaleInfo& other)
{
	m_iIndex = other.m_iIndex;
	m_oName = other.m_oName;
	m_oFileName = other.m_oFileName;
	m_eWordBreakStyle = other.m_eWordBreakStyle;
	return *this;
}

int TLocaleInfo::GetIndex() const
{
	return m_iIndex;
}

TPCString TLocaleInfo::GetName() const
{
	return m_oName;
}

TPCString TLocaleInfo::GetFileName() const
{
	return m_oFileName;
}

TLocaleInfo::WordBreakStyle TLocaleInfo::GetWordBreakStyle() const
{
	return m_eWordBreakStyle;
}

// --- TLocalisedString ----------------------------------------------------------------------

void* TSTDCALL TLocalisedString::operator new(std::size_t size)
{
	return ms_oFreelist.New(static_cast<unsigned int>(size));
}

void* TSTDCALL TLocalisedString::operator new(std::size_t size, char*, int)
{
	return ms_oFreelist.New(static_cast<unsigned int>(size));
}

void* TSTDCALL TLocalisedString::operator new(std::size_t, void* place)
{
	return place;
}

void TSTDCALL TLocalisedString::operator delete(void* memory)
{
	ms_oFreelist.Delete(memory);
}

TFreeList& TSTDCALL TLocalisedString::GetFreeList()
{
	return ms_oFreelist;
}

TLocalisedString::TLocalisedString() : m_iRefCount(0), m_iID(-1), m_pLocale(nullptr)
{
}

TLocalisedString::TLocalisedString(int id, TPCString identifier, TLocale* locale)
    : m_iRefCount(0), m_iID(id), m_oIdentifier(identifier), m_pLocale(locale)
{
}

TLocalisedString::~TLocalisedString()
{
	if (m_pLocale != nullptr)
		m_pLocale->Remove(*this);
}

void TLocalisedString::Delete()
{
	delete this;
}

int TLocalisedString::GetIdentifierInt() const
{
	return m_iID;
}

const TPCString& TLocalisedString::GetIdentifierString() const
{
	return m_oIdentifier;
}

const TPWString& TLocalisedString::GetLocalisedString() const
{
	return m_oString;
}

bool TLocalisedString::IsTemporary() const
{
	return m_iID >= TLocale::TEMP_START_INDEX;
}

// --- TLString ------------------------------------------------------------------------------

TLString::TLString() : m_pString(nullptr)
{
}

TLString::TLString(TLocalisedString* string) : m_pString(string)
{
	if (string != nullptr)
		++string->m_iRefCount;
}

TLString::TLString(const TLString& other) : m_pString(other.m_pString)
{
	if (m_pString != nullptr)
		++m_pString->m_iRefCount;
}

TLString::~TLString()
{
	if (m_pString != nullptr && --m_pString->m_iRefCount == 0)
		m_pString->Delete();
	m_pString = nullptr;
}

TLString& TLString::operator=(const TLString& other)
{
	if (m_pString != other.m_pString)
	{
		if (m_pString != nullptr && --m_pString->m_iRefCount == 0)
			m_pString->Delete();
		m_pString = other.m_pString;
		if (m_pString != nullptr)
			++m_pString->m_iRefCount;
	}
	return *this;
}

int TLString::Compare(const TLString& other) const
{
	return static_cast<int>(m_pString - other.m_pString);
}

bool TLString::operator==(const TLString& other) const
{
	return m_pString == other.m_pString;
}

bool TLString::operator!=(const TLString& other) const
{
	return m_pString != other.m_pString;
}

bool TLString::operator<(const TLString& other) const
{
	return Compare(other) < 0;
}

const TWString& TLString::GetLocalisedString() const
{
	if (m_pString == nullptr)
		return ms_wsEmpty;
	return m_pString->m_oString.GetWString();
}

const TCString& TLString::GetIdentifierString() const
{
	if (m_pString == nullptr)
		return ms_csEmpty;
	return m_pString->m_oIdentifier.GetCString();
}

TWString& TLString::GetVolatileString() const
{
	return const_cast<TWString&>(m_pString->m_oString.GetWString());
}

TLocale* TLString::GetLocale() const
{
	return m_pString == nullptr ? nullptr : m_pString->m_pLocale;
}

bool TLString::IsTemporary() const
{
	return m_pString != nullptr && m_pString->IsTemporary();
}

TLString::operator const TWString*() const
{
	return &GetLocalisedString();
}

const TWString* TLString::operator->() const
{
	return &GetLocalisedString();
}

const TWString& TLString::operator*() const
{
	return GetLocalisedString();
}

// --- TLocaleChangeEvent --------------------------------------------------------------------

TLocaleChangeEvent::TLocaleChangeEvent(TLocale* locale, TLocaleInfo* info) : m_pLocale(locale), m_pInfo(info)
{
}

TLocaleChangeEvent& TLocaleChangeEvent::operator=(const TLocaleChangeEvent& other)
{
	m_pLocale = other.m_pLocale;
	m_pInfo = other.m_pInfo;
	return *this;
}

// --- TLocale -------------------------------------------------------------------------------

TLocale::TLocale()
    : m_oEmitter(this), m_iNextTemporary(TEMP_START_INDEX), m_oNamedStrings(0x80), m_oNumberedStrings(0x80, 3000),
      m_oLocaleInfos(10), m_pCurrentLocaleInfo(nullptr)
{
}

// The emitter's links are copied as they are, as in the original.
TLocale::TLocale(const TLocale& other)
    : m_oEmitter(other.m_oEmitter), m_iNextTemporary(other.m_iNextTemporary), m_oNamedStrings(other.m_oNamedStrings),
      m_oNumberedStrings(other.m_oNumberedStrings), m_oLocaleInfos(other.m_oLocaleInfos),
      m_oDirectory(other.m_oDirectory), m_pCurrentLocaleInfo(other.m_pCurrentLocaleInfo)
{
}

// Quirk kept from the original: deleting a string removes it from the map being walked, so
// every other string is skipped (and stays allocated). The locale infos are not deleted.
TLocale::~TLocale()
{
	for (int i = 0; i < m_oNamedStrings.Count(); ++i)
		if (m_oNamedStrings[i].second != nullptr)
			m_oNamedStrings[i].second->Delete();
	for (int i = 0; i < m_oNumberedStrings.Count(); ++i)
		if (m_oNumberedStrings[i].second != nullptr)
			m_oNumberedStrings[i].second->Delete();
}

TLocale& TLocale::operator=(TLocale& other)
{
	m_oEmitter = other.m_oEmitter;
	m_iNextTemporary = other.m_iNextTemporary;
	m_oNamedStrings = other.m_oNamedStrings;
	m_oNumberedStrings = other.m_oNumberedStrings;
	m_oLocaleInfos = other.m_oLocaleInfos;
	m_oDirectory = other.m_oDirectory;
	m_pCurrentLocaleInfo = other.m_pCurrentLocaleInfo;
	return *this;
}

// Only strings held by nothing but the map (one reference) are deleted, with the same
// skipping as the destructor.
void TLocale::Reset()
{
	auto release = [](TLocalisedString* string)
	{
		if (string != nullptr && string->m_iID < TEMP_START_INDEX && string->m_iRefCount == 1)
		{
			string->m_iRefCount = 0;
			string->Delete();
		}
	};
	for (int i = 0; i < m_oNamedStrings.Count(); ++i)
		release(m_oNamedStrings[i].second);
	for (int i = 0; i < m_oNumberedStrings.Count(); ++i)
		release(m_oNumberedStrings[i].second);
}

namespace
{

// The pooled string of a token, if it carries one (the original reads the token's value as a
// string handle whatever its type).
template <typename Token> auto TokenString(const Token& token)
{
	using String = std::remove_cvref_t<decltype(token.GetString())>;
	const TFileLexer::TokenType type = token.GetType();
	if (type == TFileLexer::TOKEN_IDENT || type == TFileLexer::TOKEN_STRING || type == TFileLexer::TOKEN_COMMENT)
		return token.GetString();
	return String();
}

} // namespace

// Entries are three tokens: the locale name (an identifier), its string file and a separator.
// Infos get their position in the file as index; repeated names are kept. A missing file ends
// the load (the original crashes).
bool TLocale::LoadInfo(const TCString& fileName)
{
	TSystem::GetWStringPool();
	int index = 0;
	m_oLocaleInfos.Clear();
	TFile* file = TFile::Create(fileName, 1);
	TCString name;
	TFile::SplitPath(fileName, m_oDirectory, name);
	if (file == nullptr)
		return true;
	TFileLexerUTF8 lexer(file, 2);
	TFileLexerUTF8::Token token;
	token = lexer.GetNextToken();
	while (token.GetType() != TFileLexer::TOKEN_EOF)
	{
		const TPCString localeName = token.GetType() == TFileLexer::TOKEN_IDENT ? token.GetString() : TPCString();
		token = lexer.GetNextToken();
		const TPCString localeFile = TokenString(token);
		token = lexer.GetNextToken();
		m_oLocaleInfos.Insert(localeName, new TLocaleInfo(index, localeName, localeFile));
		++index;
		token = lexer.GetNextToken();
	}
	TFile::Destroy(file);
	return true;
}

TLocaleInfo* TLocale::AddLocale(const TPCString& name)
{
	const int index = m_oLocaleInfos.FindFirst(name);
	if (index != -1)
		return m_oLocaleInfos[index].second;
	auto* info = new TLocaleInfo(m_oLocaleInfos.Count(), name, TPCString());
	m_oLocaleInfos.Insert(name, info);
	return info;
}

TLocaleInfo* TLocale::FindLocaleInfo(const TPCString& name)
{
	const int index = m_oLocaleInfos.FindFirst(name);
	return index == -1 ? nullptr : m_oLocaleInfos[index].second;
}

TLocaleInfo* TLocale::FindLocaleInfo(const char* name)
{
	return FindLocaleInfo(TSystem::GetCStringPool()->Get(name));
}

TLocaleInfo* TLocale::GetCurrentLocaleInfo()
{
	return m_pCurrentLocaleInfo;
}

void TLocale::ChangeLocale(TLocaleInfo* info)
{
	if (!info->GetFileName().GetCString().IsEmpty())
		Load(m_oDirectory + info->m_oFileName);
	m_pCurrentLocaleInfo = info;
	TLocaleChangeEvent event(this, info);
	m_oEmitter.Throw(&event);
}

// An unknown name passes a null info (the original dereferences a null entry).
void TLocale::ChangeLocale(const TPCString& name)
{
	const int index = m_oLocaleInfos.FindFirst(name);
	ChangeLocale(index < 0 ? nullptr : m_oLocaleInfos[index].second);
}

void TLocale::ChangeLocale(const char* name)
{
	ChangeLocale(TSystem::GetCStringPool()->Get(name));
}

void TLocale::Dump() const
{
	for (int i = 0; i < m_oNamedStrings.Count(); ++i)
	{
		const TLocalisedString* string = m_oNamedStrings[i].second;
		const TWString name(string->m_oIdentifier.GetCString().GetString(0));
		TWString line;
		const unsigned short format[] = {'%', 's', ':', ' ', '%', 's', '\n', 0};
		line.Format(format, name.GetString(0), string->m_oString.GetWString().GetString(0)).Print();
	}
	for (int i = 0; i < m_oNumberedStrings.Count(); ++i)
	{
		const TLocalisedString* string = m_oNumberedStrings[i].second;
		TWString line;
		const unsigned short format[] = {'%', 'd', ':', ' ', '%', 's', '\n', 0};
		line.Format(format, string->m_iID, string->m_oString.GetWString().GetString(0)).Print();
	}
}

bool TLocale::IsValid(const TPCString& name)
{
	bool found = false;
	m_oNamedStrings.Search(name, found);
	return found;
}

bool TLocale::IsValid(const char* name)
{
	return IsValid(TSystem::GetCStringPool()->Get(name));
}

bool TLocale::IsValid(int id)
{
	bool found = false;
	m_oNumberedStrings.Search(id, found);
	return found;
}

TLString TLocale::Get(const TPCString& name)
{
	bool found = false;
	const int index = m_oNamedStrings.Search(name, found);
	return found ? TLString(m_oNamedStrings[index - 1].second) : TLString();
}

TLString TLocale::Get(const char* name)
{
	return Get(TSystem::GetCStringPool()->Get(name));
}

TLString TLocale::Get(int id)
{
	bool found = false;
	const int index = m_oNumberedStrings.Search(id, found);
	return found ? TLString(m_oNumberedStrings[index - 1].second) : TLString();
}

TLString TLocale::GetOptional(const TPCString& name)
{
	return Get(name);
}

TLString TLocale::GetOptional(const char* name)
{
	const TPCString string = TSystem::GetCStringPool()->GetOptional(name);
	if (string.IsEmpty())
		return TLString();
	return GetOptional(string);
}

TLocalisedString* TLocale::FindTemporary(const TPWString& text)
{
	for (int i = 0; i < m_oNumberedStrings.Count(); ++i)
	{
		TLocalisedString* string = m_oNumberedStrings[i].second;
		if (string->m_iID >= TEMP_START_INDEX && string->m_oString == text)
			return string;
	}
	return nullptr;
}

TLString TLocale::GetTemporary(const unsigned short* text)
{
	const TPWString pooled = TSystem::GetWStringPool()->Get(text);
	TLocalisedString* string = FindTemporary(pooled);
	if (string == nullptr)
	{
		string = new TLocalisedString(m_iNextTemporary, TPCString(), this);
		string->m_oString = pooled;
		m_oNumberedStrings.Insert(m_iNextTemporary, string);
		++m_iNextTemporary;
	}
	return TLString(string);
}

TLString TLocale::GetTemporary(const TWString& text)
{
	return GetTemporary(text.GetString(0));
}

TLString TLocale::GetTemporary(int value)
{
	TWString text;
	const unsigned short format[] = {'%', 'd', 0};
	text.Format(format, value);
	return GetTemporary(text);
}

TLString TLocale::GetVolatile(const unsigned short* text)
{
	auto* string = new TLocalisedString(m_iNextTemporary, TPCString(), this);
	string->m_oString = TSystem::GetWStringPool()->GetVolatile(text);
	m_oNumberedStrings.Insert(m_iNextTemporary, string);
	++m_iNextTemporary;
	return TLString(string);
}

TLString TLocale::GetVolatile(const TWString& text)
{
	return GetVolatile(text.GetString(0));
}

TLString TLocale::GetVolatile(int value)
{
	TWString text;
	const unsigned short format[] = {'%', 'd', 0};
	text.Format(format, value);
	return GetVolatile(text);
}

namespace
{

TLString FormatTemporary(TLocale& locale, const TLString& format, va_list args)
{
	TWString text;
	text.VFormat(format.GetLocalisedString().GetString(0), args);
	return locale.GetTemporary(text);
}

} // namespace

TLString __cdecl TLocale::Format(const TPCString& name, ...)
{
	const TLString format = Get(name);
	// va_start cannot take a reference parameter. The variadic arguments follow the return
	// address, `this` and `name` (x86 __cdecl passes them on the stack; x64 homes them).
	auto* args = static_cast<char*>(_AddressOfReturnAddress()) + 3 * sizeof(void*);
	return FormatTemporary(*this, format, args);
}

TLString __cdecl TLocale::Format(const char* name, ...)
{
	const TLString format = Get(name);
	va_list args;
	va_start(args, name);
	TLString result = FormatTemporary(*this, format, args);
	va_end(args);
	return result;
}

TLString __cdecl TLocale::Format(int id, ...)
{
	const TLString format = Get(id);
	va_list args;
	va_start(args, id);
	TLString result = FormatTemporary(*this, format, args);
	va_end(args);
	return result;
}

// Loaded strings are pinned with one reference held for the map.
TLocalisedString* TLocale::Insert(const TPCString& name, const TPWString& text)
{
	TLocalisedString* string;
	const int index = m_oNamedStrings.FindFirst(name);
	if (index == -1)
	{
		string = new TLocalisedString(-1, name, this);
		m_oNamedStrings.Insert(name, string);
		++string->m_iRefCount;
	}
	else
		string = m_oNamedStrings[index].second;
	string->m_oString = text;
	return string;
}

TLocalisedString* TLocale::Insert(const char* name, const TPWString& text)
{
	return Insert(TSystem::GetCStringPool()->Get(name), text);
}

TLocalisedString* TLocale::Insert(int id, const TPWString& text)
{
	TLocalisedString* string;
	const int index = m_oNumberedStrings.FindFirst(id);
	if (index == -1)
	{
		string = new TLocalisedString(id, TPCString(), this);
		m_oNumberedStrings.Insert(id, string);
		++string->m_iRefCount;
	}
	else
		string = m_oNumberedStrings[index].second;
	string->m_oString = text;
	return string;
}

// Entries are three tokens: an identifier (named string) or a positive number (numbered
// string), the text and a separator. A missing file ends the load (the original crashes).
bool TLocale::Load(const TCString& fileName)
{
	TSystem::GetWStringPool();
	TFile* file = TFile::Create(fileName, 1);
	if (file == nullptr)
		return true;
	TFileLexerUTF16 lexer(file, 2);
	TFileLexerUTF16::Token token;
	token = lexer.GetNextToken();
	while (token.GetType() != TFileLexer::TOKEN_EOF)
	{
		TPCString name;
		int id = -1;
		if (token.GetType() == TFileLexer::TOKEN_INTEGER)
			id = token.GetInteger();
		else if (token.GetType() == TFileLexer::TOKEN_IDENT)
			name = TSystem::GetCStringPool()->Get(TCString(token.GetString().GetWString()));
		token = lexer.GetNextToken();
		const TPWString text = TokenString(token);
		token = lexer.GetNextToken();
		if (id < 1)
		{
			if (!name.GetCString().IsEmpty())
				Insert(name, text);
		}
		else
			Insert(id, text);
		token = lexer.GetNextToken();
	}
	TFile::Destroy(file);
	return true;
}

void TLocale::Remove(TLocalisedString& string)
{
	if (string.m_oIdentifier.GetCString().IsEmpty())
		m_oNumberedStrings.RemoveAt(m_oNumberedStrings.FindFirst(string.m_iID));
	else
		m_oNamedStrings.RemoveAt(m_oNamedStrings.FindFirst(string.m_oIdentifier));
}

const TLocale::NamedStrings& TLocale::GetNamedStrings()
{
	return m_oNamedStrings;
}

const TLocale::NumberedStrings& TLocale::GetNumberedStrings()
{
	return m_oNumberedStrings;
}

const TLocale::LocaleInfos& TLocale::GetAvailLocaleInfo()
{
	return m_oLocaleInfos;
}

} // namespace Toshi
