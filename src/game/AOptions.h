#pragma once

#include "common/abi.h"
#include "toshi/kernel/TEvent.h"
#include "toshi/kernel/strings/TCString.h"
#include "toshi/kernel/strings/TCStringPool.h"
#include "toshi/kernel/strings/TWStringPool.h"

class PProperties;
class PPropertyValue;

namespace Toshi
{
class TVector2;
class TVector3;
} // namespace Toshi

class AOptions;

// Sent after the options file has been loaded.
class AOptionLoadEvent
{
};

// Sent after an option has been put; carries its name.
class AOptionChangeEvent
{
public:
	Toshi::TPCString m_oName;
};

// The game's options (SimJP.exe): a property tree loaded from and saved to "/Options.ini"
// (PPropertyParser format). Options are read and written in the current group, a property set
// that GetGroup/PutGroup enter and CloseGroup leaves. A singleton (GetOptions).
class AOptions
{
public:
	// Our names; the original only exports the type.
	enum Result
	{
		RESULT_OK = 0,
		RESULT_SAVE_FAILED = 2,
		RESULT_LOAD_FAILED = 3,
	};

	AOptions();
	// Quirk: copies the emitters' listener links and the tree pointers (the copy shares the
	// tree), as the original.
	AOptions(const AOptions& other);
	virtual ~AOptions();
	AOptions& operator=(const AOptions& other);

	// Loads the options file; throws the load event when it was read. Then, unless a locale is
	// already active, switches to the "Locale" option's locale (English when there is none).
	Result LoadOptions();
	Result SaveOptions();

	// Enters the sub-set `name` of the current group (false when there is none). PutGroup
	// creates it when it is missing.
	bool GetGroup(const char* name);
	bool PutGroup(const char* name);
	// Back to the parent set. Quirk: no check for the top level, as the original.
	void CloseGroup();

	// False when the option is missing or has another type.
	bool GetOption(const char* name, int& value);
	bool GetOption(const char* name, unsigned int& value);
	bool GetOption(const char* name, float& value);
	bool GetOption(const char* name, bool& value);
	bool GetOption(const char* name, Toshi::TPCString& value);
	bool GetOption(const char* name, Toshi::TPWString& value);
	bool GetOption(const char* name, Toshi::TVector2& value);
	bool GetOption(const char* name, Toshi::TVector3& value);
	bool GetOption(const char* name, PPropertyValue& value);
	// `count` integers from an array option.
	bool GetOption(const char* name, unsigned int* values, const int& count);

	// Replaces or adds the option, then throws the change event.
	void PutOption(const char* name, const int& value);
	void PutOption(const char* name, const unsigned int& value);
	void PutOption(const char* name, const float& value);
	void PutOption(const char* name, const bool& value);
	void PutOption(const char* name, const Toshi::TPCString& value);
	void PutOption(const char* name, const Toshi::TPWString& value);
	void PutOption(const char* name, const Toshi::TVector2& value);
	void PutOption(const char* name, const Toshi::TVector3& value);
	void PutOption(const char* name, const PPropertyValue& value);
	// An array of `count` unsigned values; an existing array of that length is updated in place.
	// Nothing happens for count 0.
	void PutOption(const char* name, const unsigned int* values, const int& count);

	int GetAutoSaveState() const;
	void SetAutoSaveState(int state);
	PProperties* GetCurrentProperties();
	Toshi::TEmitter<AOptions, AOptionChangeEvent>* GetOptionChangeEmitter();
	Toshi::TEmitter<AOptions, AOptionLoadEvent>* GetOptionLoadEmitter();

	static AOptions* TSTDCALL GetOptions();

protected:
	// `port` and `slot` are unused (console memory card leftovers). The file is "dir/name.ini".
	Result LoadOptions(int port, int slot, const Toshi::TCString& dir, const Toshi::TCString& name);
	Result SaveOptions(int port, int slot, const Toshi::TCString& dir, const Toshi::TCString& name);

private:
	void ThrowChange(const Toshi::TPCString& name);

	static AOptions* ms_pSingleton;
	static const int sm_iPort;
	static const int sm_iSlot;
	static const char sm_szOptionsDir[];
	static const char sm_szOptionsName[];

	Toshi::TEmitter<AOptions, AOptionLoadEvent> m_oLoadEmitter;
	Toshi::TEmitter<AOptions, AOptionChangeEvent> m_oChangeEmitter;
	// 1 when created, 2 after a save; the game also stores its own states.
	int m_iAutoSaveState;
	PProperties* m_pRoot;
	PProperties* m_pCurrent;
};
