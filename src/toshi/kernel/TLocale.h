#pragma once

#include "common/abi.h"
#include "common/layout.h"
#include "toshi/kernel/TCStringPool.h"
#include "toshi/kernel/TEvent.h"
#include "toshi/kernel/TFreeList.h"
#include "toshi/kernel/TMap.h"
#include "toshi/kernel/TWStringPool.h"

#include <cstddef>

// PPropertyParser.dll reads a string's numeric id inline.
class PPropertyWriter;

namespace Toshi
{

class TLocale;

// A language the locale can switch to: its name and string file (TKernelInterface.dll).
class TLocaleInfo
{
public:
	enum WordBreakStyle
	{
		WORDBREAK_SPACES,
		WORDBREAK_ANYWHERE, // Japanese
	};

	TLocaleInfo(int index, const TPCString& name, const TPCString& fileName);
	TLocaleInfo(const TLocaleInfo& other);
	~TLocaleInfo();
	TLocaleInfo& operator=(const TLocaleInfo& other);

	int GetIndex() const;
	TPCString GetName() const;
	TPCString GetFileName() const;
	WordBreakStyle GetWordBreakStyle() const;

private:
	friend class TLocale;

	int m_iIndex;
	TPCString m_oName;
	TPCString m_oFileName;
	WordBreakStyle m_eWordBreakStyle;
};

JPOG_ASSERT_SIZE(TLocaleInfo, 0x10);

// One localised string: a named or numbered identifier and its text. Reference counted by
// TLString handles (and by the locale's maps for loaded strings); allocated from a free list.
class TLocalisedString
{
	friend class TLocale;
	friend class TLString;

public:
	static void* TSTDCALL operator new(std::size_t size);
	static void* TSTDCALL operator new(std::size_t size, char* file, int line);
	static void* TSTDCALL operator new(std::size_t size, void* place);
	static void TSTDCALL operator delete(void* memory);
	static TFreeList& TSTDCALL GetFreeList();

	int GetIdentifierInt() const;
	const TPCString& GetIdentifierString() const;
	const TPWString& GetLocalisedString() const;
	// Temporary and volatile strings have identifiers from TLocale::TEMP_START_INDEX up.
	bool IsTemporary() const;

private:
	TLocalisedString();
	TLocalisedString(int id, TPCString identifier, TLocale* locale);
	// Removes the string from its locale's maps.
	~TLocalisedString();
	void Delete();

	static TFreeList ms_oFreelist;

	int m_iRefCount;
	int m_iID;
	TPCString m_oIdentifier;
	TPWString m_oString;
	TLocale* m_pLocale;
};

JPOG_ASSERT_SIZE(TLocalisedString, 0x14);

// Handle to a localised string; a null handle reads as empty strings.
class TLString
{
	friend class ::PPropertyWriter;

public:
	TLString();
	TLString(TLocalisedString* string);
	TLString(const TLString& other);
	~TLString();
	TLString& operator=(const TLString& other);

	// Orders by the strings' addresses (in units of TLocalisedString), as the original.
	int Compare(const TLString& other) const;
	bool operator==(const TLString& other) const;
	bool operator!=(const TLString& other) const;
	bool operator<(const TLString& other) const;

	const TWString& GetLocalisedString() const;
	const TCString& GetIdentifierString() const;
	// The text itself; the handle and its text must not be null.
	TWString& GetVolatileString() const;
	TLocale* GetLocale() const;
	bool IsTemporary() const;

	operator const TWString*() const;
	const TWString* operator->() const;
	const TWString& operator*() const;

	static const TCString ms_csEmpty;
	static const TWString ms_wsEmpty;

private:
	TLocalisedString* m_pString;
};

JPOG_ASSERT_SIZE(TLString, 4);

// Sent to the locale's listeners after ChangeLocale.
class TLocaleChangeEvent
{
public:
	TLocaleChangeEvent(TLocale* locale, TLocaleInfo* info);
	TLocaleChangeEvent& operator=(const TLocaleChangeEvent& other);

	TLocale* m_pLocale;
	TLocaleInfo* m_pInfo;
};

// The string table (TKernelInterface.dll). LoadInfo reads the list of locales (name and string
// file per entry); ChangeLocale loads a locale's strings, addressed by name or number. Temporary
// strings (formatted or ad-hoc text) get numbers from TEMP_START_INDEX up.
class TLocale
{
public:
	using NamedStrings = TMap<TPCString, TLocalisedString*, TPCString::Comparator>;
	using NumberedStrings = TMap<int, TLocalisedString*, TComparator<int>>;
	using LocaleInfos = TMap<TPCString, TLocaleInfo*, TPCString::Comparator>;

	TLocale();
	// Shallow copies (the strings are shared).
	TLocale(const TLocale& other);
	~TLocale();
	TLocale& operator=(TLocale& other);

	// Reads "Name file.txt ;" style entries; the directory of `fileName` is where the string
	// files are looked up.
	bool LoadInfo(const TCString& fileName);
	TLocaleInfo* AddLocale(const TPCString& name);
	TLocaleInfo* FindLocaleInfo(const TPCString& name);
	TLocaleInfo* FindLocaleInfo(const char* name);
	TLocaleInfo* GetCurrentLocaleInfo();
	// Loads the locale's string file (if it has one) and notifies the listeners.
	void ChangeLocale(TLocaleInfo* info);
	void ChangeLocale(const TPCString& name);
	void ChangeLocale(const char* name);
	// Deletes the loaded strings no handle refers to.
	void Reset();
	void Dump() const;

	bool IsValid(const TPCString& name);
	bool IsValid(const char* name);
	bool IsValid(int id);
	// The last string with the identifier, or a null handle.
	TLString Get(const TPCString& name);
	TLString Get(const char* name);
	TLString Get(int id);
	TLString GetOptional(const TPCString& name);
	TLString GetOptional(const char* name);
	// A temporary string with the text (shared with an existing one of the same text).
	TLString GetTemporary(const unsigned short* text);
	TLString GetTemporary(const TWString& text);
	TLString GetTemporary(int value);
	// A new temporary string with its own copy of the text.
	TLString GetVolatile(const unsigned short* text);
	TLString GetVolatile(const TWString& text);
	TLString GetVolatile(int value);
	// The named or numbered string formatted with the arguments, as a temporary string.
	TLString __cdecl Format(const TPCString& name, ...);
	TLString __cdecl Format(const char* name, ...);
	TLString __cdecl Format(int id, ...);
	// Sets the text of the named or numbered string, creating it if needed.
	TLocalisedString* Insert(const TPCString& name, const TPWString& text);
	TLocalisedString* Insert(const char* name, const TPWString& text);
	TLocalisedString* Insert(int id, const TPWString& text);

	const NamedStrings& GetNamedStrings();
	const NumberedStrings& GetNumberedStrings();
	const LocaleInfos& GetAvailLocaleInfo();

	static const int TEMP_START_INDEX;

protected:
	// Loads "name = text" style entries (identifier or number, then the string) from a UTF-16
	// file.
	bool Load(const TCString& fileName);
	TLocalisedString* FindTemporary(const TPWString& text);
	void Remove(TLocalisedString& string);

private:
	friend class TLocalisedString;

	TEmitter<TLocale, TLocaleChangeEvent> m_oEmitter;
	int m_iNextTemporary;
	NamedStrings m_oNamedStrings;
	NumberedStrings m_oNumberedStrings;
	LocaleInfos m_oLocaleInfos;
	TCString m_oDirectory;
	TLocaleInfo* m_pCurrentLocaleInfo;
};

JPOG_ASSERT_SIZE(TLocale, 0x4C);

} // namespace Toshi
