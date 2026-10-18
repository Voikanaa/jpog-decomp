#include "ppropertyparser/PProperties.h"

#include "toshi/kernel/TCString.h"
#include "toshi/kernel/TFreeList.h"
#include "toshi/kernel/TSystem.h"

#include <new>

using namespace Toshi;

TFreeList PProperties::PProperty::ms_oFreelist(sizeof(PProperties::PProperty), 0, 8,
                                               const_cast<char*>("PProperties::PProperty"));
TFreeList PProperties::ms_oFreelist(sizeof(PProperties), 0, 8, const_cast<char*>("PProperties"));
TClass PProperties::m_sClass("PProperties", &TObject::m_sClass, CreateObject, CreateObjectInPlace, InitialiseStatic,
                             DeinitialiseStatic, 0x10000);

// --- PProperty ------------------------------------------------------------------------------

PProperties::PProperty::PProperty(const PPropertyName& name, const PPropertyValue& value)
    : m_oName(name), m_oValue(value)
{
}

PProperties::PProperty::PProperty(const PPropertyName& name, const PPropertyValue& value, const TPCString& comment)
    : m_oName(name), m_oValue(value), m_oComment(comment)
{
}

PProperties::PProperty::PProperty(const PProperty& other) = default;

PProperties::PProperty::~PProperty()
{
	Remove();
}

PProperties::PProperty& PProperties::PProperty::operator=(const PProperty& other) = default;

const PPropertyName& PProperties::PProperty::GetName() const
{
	return m_oName;
}

const PPropertyValue* PProperties::PProperty::GetValue() const
{
	return &m_oValue;
}

void PProperties::PProperty::SetValue(const PPropertyValue& value)
{
	m_oValue = value;
}

TPCString PProperties::PProperty::GetComment() const
{
	return m_oComment;
}

void PProperties::PProperty::SetComment(const TPCString& comment)
{
	m_oComment = comment;
}

void* TSTDCALL PProperties::PProperty::operator new(std::size_t size)
{
	return ms_oFreelist.New(static_cast<unsigned int>(size));
}

void* TSTDCALL PProperties::PProperty::operator new(std::size_t size, char*, int)
{
	return ms_oFreelist.New(static_cast<unsigned int>(size));
}

void* TSTDCALL PProperties::PProperty::operator new(std::size_t, void* place)
{
	return place;
}

void TSTDCALL PProperties::PProperty::operator delete(void* memory)
{
	ms_oFreelist.Delete(memory);
}

TFreeList& TSTDCALL PProperties::PProperty::GetFreeList()
{
	return ms_oFreelist;
}

// --- PProperties ----------------------------------------------------------------------------

PProperties::PProperties() : m_pParent(nullptr)
{
}

PProperties::PProperties(PProperties* parent) : m_pParent(parent)
{
}

PProperties::~PProperties()
{
	while (!m_oProperties.IsEmpty())
		delete m_oProperties.Begin().Get();
	m_oProperties.UnlinkAll();
}

TClass& PProperties::GetClass() const
{
	return m_sClass;
}

TQList<PProperties::PProperty>::Iterator PProperties::Begin() const
{
	return m_oProperties.Begin();
}

TQList<PProperties::PProperty>::Iterator PProperties::End() const
{
	return m_oProperties.End();
}

TQList<PProperties::PProperty>& PProperties::GetPropertySet()
{
	return m_oProperties;
}

const TQList<PProperties::PProperty>& PProperties::GetPropertySet() const
{
	return m_oProperties;
}

int PProperties::GetPropertyCount() const
{
	return m_oProperties.Count();
}

bool PProperties::IsEmpty() const
{
	return m_oProperties.IsEmpty();
}

PProperties* PProperties::GetParentProperties() const
{
	return m_pParent;
}

void PProperties::PutProperty(const PPropertyName& name, const PPropertyValue& value)
{
	m_oProperties.PushBack(new PProperty(name, value));
}

void PProperties::PutProperty(const PPropertyName& name, const PPropertyValue& value, const TPCString& comment)
{
	m_oProperties.PushBack(new PProperty(name, value, comment));
}

void PProperties::PutProperty(const TPCString& name, const PPropertyValue& value)
{
	PutProperty(PPropertyName(name), value);
}

void PProperties::PutProperty(const TPCString& name, const PPropertyValue& value, const TPCString& comment)
{
	PutProperty(PPropertyName(name), value, comment);
}

void PProperties::PutProperty(const TPCString& name, const TPCString& subName, const PPropertyValue& value)
{
	PutProperty(PPropertyName(name, subName), value);
}

void PProperties::PutProperty(const TPCString& name, const TPCString& subName, const PPropertyValue& value,
                              const TPCString& comment)
{
	PutProperty(PPropertyName(name, subName), value, comment);
}

void PProperties::PutPropertyUnique(const PPropertyName& name, const PPropertyValue& value)
{
	for (auto it = m_oProperties.Begin(); it != m_oProperties.End(); ++it)
		if (it->m_oName == name)
		{
			it->m_oValue = value;
			return;
		}
	m_oProperties.PushBack(new PProperty(name, value));
}

void PProperties::PutPropertyUnique(const PPropertyName& name, const PPropertyValue& value, const TPCString& comment)
{
	m_oProperties.PushBack(new PProperty(name, value, comment));
}

void PProperties::PutPropertyUnique(const TPCString& name, const PPropertyValue& value)
{
	PutPropertyUnique(PPropertyName(name), value);
}

void PProperties::PutPropertyUnique(const TPCString& name, const PPropertyValue& value, const TPCString& comment)
{
	PutPropertyUnique(PPropertyName(name), value, comment);
}

void PProperties::PutPropertyUnique(const TPCString& name, const TPCString& subName, const PPropertyValue& value)
{
	PutPropertyUnique(PPropertyName(name, subName), value);
}

void PProperties::PutPropertyUnique(const TPCString& name, const TPCString& subName, const PPropertyValue& value,
                                    const TPCString& comment)
{
	PutPropertyUnique(PPropertyName(name, subName), value, comment);
}

const PPropertyValue* PProperties::GetProperty(const PPropertyName& name) const
{
	for (auto it = m_oProperties.Begin(); it != m_oProperties.End(); ++it)
		if (it->m_oName == name)
			return &it->m_oValue;
	return nullptr;
}

const PPropertyValue* PProperties::GetProperty(const TPCString& name) const
{
	return GetProperty(PPropertyName(name));
}

const PPropertyValue* PProperties::GetProperty(const TPCString& name, const TPCString& subName) const
{
	return GetProperty(PPropertyName(name, subName));
}

const PPropertyValue* PProperties::GetProperty(const TPCString& name, int index) const
{
	TCString text;
	text.Format("%i", index);
	return GetProperty(PPropertyName(name, TSystem::GetCStringPool()->Get(text)));
}

const PPropertyValue* PProperties::GetRequiredProperty(const TPCString& name, const TClass* type) const
{
	const PPropertyValue* value = GetProperty(PPropertyName(name));
	return value != nullptr && value->GetType() == type ? value : nullptr;
}

const PPropertyValue* PProperties::GetRequiredProperty(const TPCString& name, const TPCString& subName,
                                                       const TClass* type) const
{
	const PPropertyValue* value = GetProperty(PPropertyName(name, subName));
	return value != nullptr && value->GetType() == type ? value : nullptr;
}

const PPropertyValue* PProperties::GetRequiredProperty(const TPCString& name, int index, const TClass* type) const
{
	const PPropertyValue* value = GetProperty(name, index);
	return value != nullptr && value->GetType() == type ? value : nullptr;
}

const PPropertyValue* PProperties::GetOptionalProperty(const TPCString& name, const TClass* type) const
{
	return GetRequiredProperty(name, type);
}

const PPropertyValue* PProperties::GetOptionalProperty(const TPCString& name, const TPCString& subName,
                                                       const TClass* type) const
{
	return GetRequiredProperty(name, subName, type);
}

const PPropertyValue* PProperties::GetOptionalProperty(const TPCString& name, int index, const TClass* type) const
{
	return GetRequiredProperty(name, index, type);
}

const PProperties::PProperty* PProperties::FindProperty(const TPCString& name,
                                                        TQList<PProperty>::Iterator& position) const
{
	for (; position != m_oProperties.End(); ++position)
		if (position->m_oName.GetName() == name)
		{
			const PProperty* found = position.Get();
			++position;
			return found;
		}
	position = m_oProperties.End();
	return nullptr;
}

void* TSTDCALL PProperties::operator new(std::size_t size)
{
	return ms_oFreelist.New(static_cast<unsigned int>(size));
}

void* TSTDCALL PProperties::operator new(std::size_t size, char*, int)
{
	return ms_oFreelist.New(static_cast<unsigned int>(size));
}

void* TSTDCALL PProperties::operator new(std::size_t, void* place)
{
	return place;
}

void TSTDCALL PProperties::operator delete(void* memory)
{
	ms_oFreelist.Delete(memory);
}

TFreeList& TSTDCALL PProperties::GetFreeList()
{
	return ms_oFreelist;
}

TObject* TSTDCALL PProperties::CreateObject()
{
	return new PProperties();
}

TObject* TSTDCALL PProperties::CreateObjectInPlace(void* memory)
{
	return memory == nullptr ? nullptr : ::new (memory) PProperties();
}

void TSTDCALL PProperties::InitialiseStatic()
{
}

void TSTDCALL PProperties::DeinitialiseStatic()
{
}
