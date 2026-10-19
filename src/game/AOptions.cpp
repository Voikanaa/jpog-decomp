#include "game/AOptions.h"

#include "ppropertyparser/PProperties.h"
#include "ppropertyparser/PPropertyReader.h"
#include "ppropertyparser/PPropertyValue.h"
#include "ppropertyparser/PPropertyWriter.h"
#include "toshi/kernel/TLocale.h"
#include "toshi/kernel/TSystem.h"
#include "toshi/kernel/TVector2.h"
#include "toshi/kernel/TVector3.h"
#include "toshi/kernel/TWString.h"

using namespace Toshi;

AOptions* AOptions::ms_pSingleton = nullptr;
const int AOptions::sm_iPort = -1;
const int AOptions::sm_iSlot = -1;
const char AOptions::sm_szOptionsDir[] = "";
const char AOptions::sm_szOptionsName[] = "Options";

namespace
{

TPCString Pooled(const char* name)
{
	return TSystem::GetCStringPool()->Get(name);
}

// The option `name` of the current group, or null.
const PPropertyValue* Find(PProperties* group, const char* name)
{
	return group->GetProperty(Pooled(name));
}

} // namespace

AOptions::AOptions()
    : m_oLoadEmitter(this), m_oChangeEmitter(this), m_iAutoSaveState(1), m_pRoot(new PProperties()), m_pCurrent(m_pRoot)
{
	ms_pSingleton = this;
}

AOptions::AOptions(const AOptions& other) = default;

AOptions::~AOptions()
{
	delete m_pRoot;
	m_pRoot = nullptr;
	m_pCurrent = nullptr;
	ms_pSingleton = nullptr;
}

AOptions& AOptions::operator=(const AOptions& other) = default;

AOptions::Result AOptions::LoadOptions()
{
	const Result result = LoadOptions(sm_iPort, sm_iSlot, TCString(sm_szOptionsDir), TCString(sm_szOptionsName));
	if (result == RESULT_OK)
	{
		AOptionLoadEvent event;
		m_oLoadEmitter.Throw(&event);
	}

	TLocale* locale = TSystem::GetLocale();
	if (locale->GetCurrentLocaleInfo() != nullptr)
		return result;
	TPCString name;
	TLocaleInfo* info = GetOption("Locale", name) ? locale->FindLocaleInfo(name) : locale->FindLocaleInfo("English");
	if (info != nullptr)
		locale->ChangeLocale(info);
	else
		locale->ChangeLocale("English");
	return result;
}

AOptions::Result AOptions::SaveOptions()
{
	return SaveOptions(sm_iPort, sm_iSlot, TCString(sm_szOptionsDir), TCString(sm_szOptionsName));
}

AOptions::Result AOptions::LoadOptions(int, int, const TCString& dir, const TCString& name)
{
	PPropertyReader reader;
	TCString path;
	path.Format("%s/%s.ini", dir.GetString(0), name.GetString(0));
	if (!reader.Open(path))
		return RESULT_LOAD_FAILED;
	// Quirk: the set leaks when it cannot be read, as in the original.
	PProperties* properties = new PProperties();
	if (!reader.LoadPropertyBlock(*properties))
		return RESULT_LOAD_FAILED;
	delete m_pRoot;
	m_pRoot = properties;
	m_pCurrent = properties;
	return RESULT_OK;
}

AOptions::Result AOptions::SaveOptions(int, int, const TCString& dir, const TCString& name)
{
	PPropertyWriter writer;
	TCString path;
	path.Format("%s/%s.ini", dir.GetString(0), name.GetString(0));
	if (!writer.Open(TWString(path)))
		return RESULT_SAVE_FAILED;
	if (!writer.PutPropertyBlock(m_pRoot))
		return RESULT_SAVE_FAILED;
	writer.Close();
	m_iAutoSaveState = 2;
	return RESULT_OK;
}

bool AOptions::GetGroup(const char* name)
{
	const PPropertyValue* value = Find(m_pCurrent, name);
	if (value == nullptr || !value->CanBeType(PPropertyValue::TYPE_PROPS))
		return false;
	m_pCurrent = value->GetProperties();
	return true;
}

bool AOptions::PutGroup(const char* name)
{
	const TPCString pooled = Pooled(name);
	const PPropertyValue* value = m_pCurrent->GetProperty(pooled);
	if (value == nullptr)
	{
		PProperties* group = new PProperties(m_pCurrent);
		m_pCurrent->PutProperty(pooled, PPropertyValue(group));
		m_pCurrent = group;
		return true;
	}
	if (!value->CanBeType(PPropertyValue::TYPE_PROPS))
		return false;
	m_pCurrent = value->GetProperties();
	return true;
}

void AOptions::CloseGroup()
{
	m_pCurrent = m_pCurrent->GetParentProperties();
}

bool AOptions::GetOption(const char* name, int& value)
{
	const PPropertyValue* option = Find(m_pCurrent, name);
	if (option == nullptr || !option->CanBeType(PPropertyValue::TYPE_INT))
		return false;
	value = option->GetInteger();
	return true;
}

bool AOptions::GetOption(const char* name, unsigned int& value)
{
	const PPropertyValue* option = Find(m_pCurrent, name);
	if (option == nullptr || !option->CanBeType(PPropertyValue::TYPE_UINT32))
		return false;
	value = option->GetUINT32();
	return true;
}

bool AOptions::GetOption(const char* name, float& value)
{
	const PPropertyValue* option = Find(m_pCurrent, name);
	if (option == nullptr || !option->CanBeType(PPropertyValue::TYPE_FLOAT))
		return false;
	value = option->GetFloat();
	return true;
}

bool AOptions::GetOption(const char* name, bool& value)
{
	const PPropertyValue* option = Find(m_pCurrent, name);
	if (option == nullptr || !option->CanBeType(PPropertyValue::TYPE_BOOL))
		return false;
	value = option->GetBoolean();
	return true;
}

bool AOptions::GetOption(const char* name, TPCString& value)
{
	const PPropertyValue* option = Find(m_pCurrent, name);
	if (option == nullptr || !option->CanBeType(PPropertyValue::TYPE_TPCSTRING))
		return false;
	value = option->GetTPCString();
	return true;
}

bool AOptions::GetOption(const char* name, TPWString& value)
{
	const PPropertyValue* option = Find(m_pCurrent, name);
	if (option == nullptr || !option->CanBeType(PPropertyValue::TYPE_TPWSTRING))
		return false;
	value = option->GetTPWString();
	return true;
}

bool AOptions::GetOption(const char* name, TVector2& value)
{
	const PPropertyValue* option = Find(m_pCurrent, name);
	if (option == nullptr || !option->IsType(PPropertyValue::TYPE_ARRAY))
		return false;
	return option->GetArray()->GetTVector2(value);
}

bool AOptions::GetOption(const char* name, TVector3& value)
{
	const PPropertyValue* option = Find(m_pCurrent, name);
	if (option == nullptr || !option->IsType(PPropertyValue::TYPE_ARRAY))
		return false;
	return option->GetArray()->GetTVector3(value);
}

bool AOptions::GetOption(const char* name, PPropertyValue& value)
{
	const PPropertyValue* option = Find(m_pCurrent, name);
	if (option == nullptr)
		return false;
	value = *option;
	return true;
}

bool AOptions::GetOption(const char* name, unsigned int* values, const int& count)
{
	if (count == 0)
		return false;
	const PPropertyValue* option = Find(m_pCurrent, name);
	if (option == nullptr || !option->IsType(PPropertyValue::TYPE_ARRAY))
		return false;
	// Deviation: the original reads `count` elements whatever the array's length; a shorter
	// array leaves the remaining values unchanged here.
	const TArray<PPropertyValue>& elements = option->GetArray()->GetValues();
	for (int i = 0; i < count && i < elements.GetNumElements(); ++i)
		values[i] = static_cast<unsigned int>(elements[i].GetInteger());
	return true;
}

void AOptions::ThrowChange(const TPCString& name)
{
	AOptionChangeEvent event{name};
	m_oChangeEmitter.Throw(&event);
}

void AOptions::PutOption(const char* name, const int& value)
{
	const TPCString pooled = Pooled(name);
	m_pCurrent->PutPropertyUnique(pooled, PPropertyValue(value));
	ThrowChange(pooled);
}

void AOptions::PutOption(const char* name, const unsigned int& value)
{
	const TPCString pooled = Pooled(name);
	m_pCurrent->PutPropertyUnique(pooled, PPropertyValue(value));
	ThrowChange(pooled);
}

void AOptions::PutOption(const char* name, const float& value)
{
	const TPCString pooled = Pooled(name);
	m_pCurrent->PutPropertyUnique(pooled, PPropertyValue(value));
	ThrowChange(pooled);
}

void AOptions::PutOption(const char* name, const bool& value)
{
	const TPCString pooled = Pooled(name);
	m_pCurrent->PutPropertyUnique(pooled, PPropertyValue(value));
	ThrowChange(pooled);
}

void AOptions::PutOption(const char* name, const TPCString& value)
{
	const TPCString pooled = Pooled(name);
	m_pCurrent->PutPropertyUnique(pooled, PPropertyValue(value));
	ThrowChange(pooled);
}

void AOptions::PutOption(const char* name, const TPWString& value)
{
	const TPCString pooled = Pooled(name);
	m_pCurrent->PutPropertyUnique(pooled, PPropertyValue(value));
	ThrowChange(pooled);
}

void AOptions::PutOption(const char* name, const TVector2& value)
{
	auto* array = new PPropertyValueArray(2);
	array->GetValues().Push(PPropertyValue(value.x));
	array->GetValues().Push(PPropertyValue(value.y));
	const TPCString pooled = Pooled(name);
	m_pCurrent->PutPropertyUnique(pooled, PPropertyValue(array));
	ThrowChange(pooled);
}

void AOptions::PutOption(const char* name, const TVector3& value)
{
	auto* array = new PPropertyValueArray(3);
	array->GetValues().Push(PPropertyValue(value.x));
	array->GetValues().Push(PPropertyValue(value.y));
	array->GetValues().Push(PPropertyValue(value.z));
	const TPCString pooled = Pooled(name);
	m_pCurrent->PutPropertyUnique(pooled, PPropertyValue(array));
	ThrowChange(pooled);
}

void AOptions::PutOption(const char* name, const PPropertyValue& value)
{
	const TPCString pooled = Pooled(name);
	m_pCurrent->PutPropertyUnique(pooled, value);
	ThrowChange(pooled);
}

void AOptions::PutOption(const char* name, const unsigned int* values, const int& count)
{
	if (count == 0)
		return;
	const TPCString pooled = Pooled(name);
	const PPropertyValue* existing = m_pCurrent->GetOptionalProperty(pooled, PPropertyValue::TYPE_ARRAY);
	PPropertyValueArray* array = existing != nullptr ? existing->GetArray() : nullptr;
	if (array != nullptr && array->GetValues().GetNumElements() == count)
	{
		for (int i = 0; i < count; ++i)
			array->GetValues()[i].SetUINT32(values[i]);
	}
	else
	{
		array = new PPropertyValueArray(count);
		for (int i = 0; i < count; ++i)
			array->GetValues().Push(PPropertyValue(values[i]));
		m_pCurrent->PutPropertyUnique(pooled, PPropertyValue(array));
	}
	ThrowChange(pooled);
}

int AOptions::GetAutoSaveState() const
{
	return m_iAutoSaveState;
}

void AOptions::SetAutoSaveState(int state)
{
	m_iAutoSaveState = state;
}

PProperties* AOptions::GetCurrentProperties()
{
	return m_pCurrent;
}

TEmitter<AOptions, AOptionChangeEvent>* AOptions::GetOptionChangeEmitter()
{
	return &m_oChangeEmitter;
}

TEmitter<AOptions, AOptionLoadEvent>* AOptions::GetOptionLoadEmitter()
{
	return &m_oLoadEmitter;
}

AOptions* __stdcall AOptions::GetOptions()
{
	return ms_pSingleton;
}
