#include "ppropertyparser/PPropertyValue.h"

#include "ppropertyparser/PProperties.h"
#include "ppropertyparser/PPropertyName.h"
#include "toshi/kernel/TObject.h"
#include "toshi/kernel/math/TQuaternion.h"
#include "toshi/kernel/math/TVector2.h"
#include "toshi/kernel/math/TVector3.h"
#include "toshi/kernel/strings/TCString.h"
#include "toshi/kernel/strings/TWString.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <new>

using namespace Toshi;

namespace
{

// The type tags: unattached classes with no factory functions.
TClass s_oIntClass("INT", nullptr, nullptr, nullptr, nullptr, nullptr, 0x10000);
TClass s_oUInt32Class("UINT32", nullptr, nullptr, nullptr, nullptr, nullptr, 0x10000);
TClass s_oFloatClass("FLOAT", nullptr, nullptr, nullptr, nullptr, nullptr, 0x10000);
TClass s_oBoolClass("BOOL", nullptr, nullptr, nullptr, nullptr, nullptr, 0x10000);
TClass s_oTPCStringClass("TPCString", nullptr, nullptr, nullptr, nullptr, nullptr, 0x10000);
TClass s_oTPWStringClass("TPWString", nullptr, nullptr, nullptr, nullptr, nullptr, 0x10000);
TClass s_oTLStringClass("TLString", nullptr, nullptr, nullptr, nullptr, nullptr, 0x10000);
TClass s_oPropNameClass("PPropertyName", nullptr, nullptr, nullptr, nullptr, nullptr, 0x10000);
TClass s_oArrayClass("PPropertyValueArray", nullptr, nullptr, nullptr, nullptr, nullptr, 0x10000);

// The original's float-to-int conversion: truncation done on the bits of the value widened to
// double. Magnitudes of 2^31 and more wrap instead of saturating.
int TruncateToInt(float value)
{
	const std::uint64_t bits = std::bit_cast<std::uint64_t>(static_cast<double>(value));
	const auto high = static_cast<std::uint32_t>(bits >> 32);
	const auto low = static_cast<std::uint32_t>(bits);
	const std::uint32_t exponent = (high >> 20) & 0x7FF;
	if (exponent < 0x3FF)
		return 0;
	std::uint32_t result = ((high << 11) | (low >> 21) | 0x80000000u) >> ((0x1Eu - exponent) & 0x1F);
	if (static_cast<std::int64_t>(bits) < 0)
		result = 0u - result;
	return static_cast<int>(result);
}

// fistp with truncation into 64 bits, keeping the low half; out of range values give the integer
// indefinite (low half 0).
unsigned int TruncateToUInt32(float value)
{
	const double truncated = std::trunc(static_cast<double>(value));
	if (!(truncated >= -9223372036854775808.0 && truncated < 9223372036854775808.0))
		return 0;
	return static_cast<unsigned int>(static_cast<std::int64_t>(truncated));
}

float ToFloat(const PPropertyValue& value)
{
	return value.GetFloat();
}

} // namespace

const TClass* const PPropertyValue::TYPE_UNDEF = nullptr;
const TClass* const PPropertyValue::TYPE_INT = &s_oIntClass;
const TClass* const PPropertyValue::TYPE_UINT32 = &s_oUInt32Class;
const TClass* const PPropertyValue::TYPE_FLOAT = &s_oFloatClass;
const TClass* const PPropertyValue::TYPE_BOOL = &s_oBoolClass;
const TClass* const PPropertyValue::TYPE_TPCSTRING = &s_oTPCStringClass;
const TClass* const PPropertyValue::TYPE_TPWSTRING = &s_oTPWStringClass;
const TClass* const PPropertyValue::TYPE_TLSTRING = &s_oTLStringClass;
const TClass* const PPropertyValue::TYPE_PROPS = &PProperties::m_sClass;
const TClass* const PPropertyValue::TYPE_PROPNAME = &s_oPropNameClass;
const TClass* const PPropertyValue::TYPE_ARRAY = &s_oArrayClass;

// --- Construction ---------------------------------------------------------------------------

PPropertyValue::PPropertyValue() : m_pType(TYPE_UNDEF), m_iValue(0)
{
}

PPropertyValue::PPropertyValue(const PPropertyValue& other) : m_pType(TYPE_UNDEF), m_iValue(0)
{
	Assign(other);
}

PPropertyValue::PPropertyValue(int value) : m_pType(TYPE_INT), m_iValue(value)
{
}

PPropertyValue::PPropertyValue(unsigned int value) : m_pType(TYPE_UINT32), m_uiValue(value)
{
}

PPropertyValue::PPropertyValue(float value) : m_pType(TYPE_FLOAT), m_fValue(value)
{
}

PPropertyValue::PPropertyValue(bool value) : m_pType(TYPE_BOOL), m_iValue(0)
{
	m_bValue = value;
}

PPropertyValue::PPropertyValue(const TPCString& value) : m_pType(TYPE_TPCSTRING), m_oTPCString(value)
{
}

PPropertyValue::PPropertyValue(const TPWString& value) : m_pType(TYPE_TPWSTRING), m_oTPWString(value)
{
}

PPropertyValue::PPropertyValue(const TLString& value) : m_pType(TYPE_TLSTRING), m_oTLString(value)
{
}

PPropertyValue::PPropertyValue(const PPropertyName& value) : m_pType(TYPE_PROPNAME), m_pName(new PPropertyName(value))
{
}

PPropertyValue::PPropertyValue(PProperties* value) : m_pType(TYPE_PROPS), m_pProperties(value)
{
}

PPropertyValue::PPropertyValue(PPropertyValueArray* value) : m_pType(TYPE_ARRAY), m_pArray(value)
{
}

PPropertyValue::PPropertyValue(TObject* value)
    : m_pType(value == nullptr ? TYPE_UNDEF : &value->GetClass()), m_pObject(value)
{
}

PPropertyValue::~PPropertyValue()
{
	ChangeType(TYPE_UNDEF);
}

bool PPropertyValue::ChangeType(const TClass* type)
{
	if (m_pType == type)
		return false;
	if (m_pType == TYPE_TPCSTRING)
		m_oTPCString.~TPCString();
	else if (m_pType == TYPE_TPWSTRING)
		m_oTPWString.~TPWString();
	else if (m_pType == TYPE_TLSTRING)
		m_oTLString.~TLString();
	else if (m_pType == TYPE_PROPS)
		m_pProperties.~TManagedPtr();
	else if (m_pType == TYPE_ARRAY)
		m_pArray.~TManagedPtr();
	else if (m_pType == TYPE_PROPNAME)
		delete m_pName;
	if (type == TYPE_TPCSTRING)
		new (&m_oTPCString) TPCString();
	else if (type == TYPE_TPWSTRING)
		new (&m_oTPWString) TPWString();
	else if (type == TYPE_TLSTRING)
		new (&m_oTLString) TLString();
	else if (type == TYPE_PROPS)
		new (&m_pProperties) TManagedPtr<PProperties>();
	else if (type == TYPE_ARRAY)
		new (&m_pArray) TManagedPtr<PPropertyValueArray>();
	else if (type == TYPE_PROPNAME)
		m_pName = nullptr;
	m_pType = type;
	return true;
}

// Quirk: assigning a name to a name leaks the old copy, as in the original.
void PPropertyValue::Assign(const PPropertyValue& other)
{
	ChangeType(other.m_pType);
	if (m_pType == TYPE_TPCSTRING)
		m_oTPCString = other.GetTPCString();
	else if (m_pType == TYPE_TPWSTRING)
		m_oTPWString = other.GetTPWString();
	else if (m_pType == TYPE_TLSTRING)
		m_oTLString = other.GetTLString();
	else if (m_pType == TYPE_INT || m_pType == TYPE_UINT32 || m_pType == TYPE_FLOAT)
		m_iValue = other.m_iValue;
	else if (m_pType == TYPE_BOOL)
		m_bValue = other.m_bValue;
	else if (m_pType == TYPE_PROPS)
		m_pProperties = other.GetProperties();
	else if (m_pType == TYPE_PROPNAME)
		m_pName = new PPropertyName(*other.m_pName);
	else if (m_pType == TYPE_ARRAY)
		m_pArray = other.m_pArray;
	else if (m_pType != TYPE_UNDEF)
		m_pObject = other.m_pObject;
}

// --- Assignment -----------------------------------------------------------------------------

PPropertyValue& PPropertyValue::operator=(const PPropertyValue& other)
{
	Assign(other);
	return *this;
}

PPropertyValue& PPropertyValue::operator=(int value)
{
	SetInteger(value);
	return *this;
}

PPropertyValue& PPropertyValue::operator=(unsigned int value)
{
	SetUINT32(value);
	return *this;
}

PPropertyValue& PPropertyValue::operator=(float value)
{
	SetFloat(value);
	return *this;
}

PPropertyValue& PPropertyValue::operator=(const TPCString& value)
{
	SetTPCString(value);
	return *this;
}

PPropertyValue& PPropertyValue::operator=(const TPWString& value)
{
	SetTPWString(value);
	return *this;
}

PPropertyValue& PPropertyValue::operator=(const TLString& value)
{
	SetTLString(value);
	return *this;
}

PPropertyValue& PPropertyValue::operator=(const PPropertyName& value)
{
	SetPropertyName(value);
	return *this;
}

PPropertyValue& PPropertyValue::operator=(PProperties* value)
{
	SetProperties(value);
	return *this;
}

PPropertyValue& PPropertyValue::operator=(PPropertyValueArray* value)
{
	SetArray(value);
	return *this;
}

// --- Comparison -----------------------------------------------------------------------------

bool PPropertyValue::operator==(const PPropertyValue& other) const
{
	if (m_pType != other.m_pType)
		return false;
	if (m_pType == TYPE_UNDEF)
		return true;
	if (m_pType == TYPE_TPCSTRING)
		return GetTPCString() == other.GetTPCString();
	if (m_pType == TYPE_TPWSTRING)
		return GetTPWString() == other.GetTPWString();
	if (m_pType == TYPE_TLSTRING)
		return GetTLString() == other.GetTLString();
	if (m_pType == TYPE_INT || m_pType == TYPE_UINT32)
		return m_iValue == other.m_iValue;
	if (m_pType == TYPE_FLOAT)
		return m_fValue == other.m_fValue;
	if (m_pType == TYPE_BOOL)
		return m_bValue == other.m_bValue;
	if (m_pType == TYPE_PROPS)
		return GetProperties() == other.GetProperties();
	if (m_pType == TYPE_PROPNAME)
		return *m_pName == *other.m_pName;
	if (m_pType == TYPE_ARRAY)
		return GetArray() == other.GetArray();
	return m_pObject == other.m_pObject;
}

bool PPropertyValue::operator!=(const PPropertyValue& other) const
{
	return !(*this == other);
}

bool PPropertyValue::operator==(int value) const
{
	return m_pType == TYPE_INT && value == m_iValue;
}

bool PPropertyValue::operator==(unsigned int value) const
{
	return m_pType == TYPE_UINT32 && value == m_uiValue;
}

bool PPropertyValue::operator==(float value) const
{
	return m_pType == TYPE_FLOAT && m_fValue == value;
}

bool PPropertyValue::operator==(bool value) const
{
	return m_pType == TYPE_BOOL && value == m_bValue;
}

bool PPropertyValue::operator==(const TPCString& value) const
{
	return m_pType == TYPE_TPCSTRING && GetTPCString() == value;
}

bool PPropertyValue::operator==(const TPWString& value) const
{
	return m_pType == TYPE_TPWSTRING && GetTPWString() == value;
}

bool PPropertyValue::operator==(const TLString& value) const
{
	return m_pType == TYPE_TLSTRING && GetTLString() == value;
}

bool PPropertyValue::operator==(const char* value) const
{
	return m_pType == TYPE_TPCSTRING && GetTPCString().GetCString().Compare(value, -1) == 0;
}

bool PPropertyValue::operator==(const unsigned short* value) const
{
	return m_pType == TYPE_TPWSTRING && GetTPWString().GetWString().Compare(value, -1) == 0;
}

bool PPropertyValue::operator==(const PPropertyName& value) const
{
	return m_pType == TYPE_PROPNAME && *m_pName == value;
}

bool PPropertyValue::operator==(const PProperties* value) const
{
	return m_pType == TYPE_PROPS && GetProperties() == value;
}

bool PPropertyValue::operator==(const PPropertyValueArray* value) const
{
	return m_pType == TYPE_ARRAY && GetArray() == value;
}

// The object's class must be the value's type (and the pointer the same).
bool PPropertyValue::operator==(const TObject* value) const
{
	return &value->GetClass() == m_pType && value == m_pObject;
}

// --- Queries --------------------------------------------------------------------------------

const TClass* PPropertyValue::GetType() const
{
	return m_pType;
}

bool PPropertyValue::IsType(const TClass* type) const
{
	return m_pType == type;
}

bool PPropertyValue::IsDefined() const
{
	return m_pType != TYPE_UNDEF;
}

// Numbers and bools are never empty; objects are empty when null.
bool PPropertyValue::IsEmpty() const
{
	if (m_pType == TYPE_TPCSTRING)
		return GetTPCString().GetCString().IsEmpty();
	if (m_pType == TYPE_TPWSTRING)
		return GetTPWString().GetWString().IsEmpty();
	if (m_pType == TYPE_TLSTRING)
		return GetTLString().GetLocalisedString().IsEmpty();
	if (m_pType == TYPE_INT || m_pType == TYPE_UINT32 || m_pType == TYPE_FLOAT || m_pType == TYPE_BOOL)
		return false;
	if (m_pType == TYPE_PROPS)
		return GetProperties() == nullptr;
	if (m_pType == TYPE_PROPNAME)
		return m_pName == nullptr || m_pName->GetName().GetCString().IsEmpty();
	if (m_pType == TYPE_ARRAY)
		return GetArray() == nullptr || GetArray()->GetValues().GetNumElements() == 0;
	if (m_pType == TYPE_UNDEF)
		return true;
	return m_pObject == nullptr;
}

// Int and uint values are compared as signed ints against the 1e8 limit, exactly.
bool PPropertyValue::CanBeType(const TClass* type) const
{
	if (m_pType == TYPE_INT || m_pType == TYPE_UINT32)
		return type == TYPE_INT || type == TYPE_UINT32 ||
		       (type == TYPE_FLOAT && static_cast<double>(m_iValue) < 100000000.0);
	if (m_pType == TYPE_FLOAT)
	{
		if (type == TYPE_FLOAT)
			return true;
		// NaN counts as equal (an unordered x87 compare reads as "equal" there).
		const double truncated = TruncateToInt(m_fValue);
		const double value = m_fValue;
		return type == TYPE_INT && !(truncated < value || truncated > value);
	}
	if (m_pType == TYPE_BOOL)
		return type == TYPE_BOOL || type == TYPE_INT || type == TYPE_FLOAT;
	return type == m_pType;
}

// --- Getters --------------------------------------------------------------------------------

int PPropertyValue::GetInteger() const
{
	if (m_pType == TYPE_INT || m_pType == TYPE_UINT32)
		return m_iValue;
	if (m_pType == TYPE_FLOAT)
		return TruncateToInt(m_fValue);
	return 0;
}

unsigned int PPropertyValue::GetUINT32() const
{
	if (m_pType == TYPE_INT || m_pType == TYPE_UINT32)
		return m_uiValue;
	if (m_pType == TYPE_FLOAT)
		return TruncateToUInt32(m_fValue);
	return 0;
}

float PPropertyValue::GetFloat() const
{
	if (m_pType == TYPE_INT)
		return static_cast<float>(m_iValue);
	if (m_pType == TYPE_UINT32)
		return static_cast<float>(m_uiValue);
	if (m_pType == TYPE_FLOAT)
		return m_fValue;
	return 0.0f;
}

bool PPropertyValue::GetBoolean() const
{
	if (m_pType == TYPE_INT || m_pType == TYPE_UINT32)
		return m_iValue != 0;
	if (m_pType == TYPE_FLOAT)
		return m_fValue < 0.0f || m_fValue > 0.0f; // NaN is false
	if (m_pType == TYPE_BOOL)
		return m_bValue;
	return false;
}

TPCString& PPropertyValue::GetTPCString()
{
	static TPCString s_oEmpty;
	return m_pType == TYPE_TPCSTRING ? m_oTPCString : s_oEmpty;
}

const TPCString& PPropertyValue::GetTPCString() const
{
	static const TPCString s_oEmpty;
	return m_pType == TYPE_TPCSTRING ? m_oTPCString : s_oEmpty;
}

TPWString& PPropertyValue::GetTPWString()
{
	static TPWString s_oEmpty;
	return m_pType == TYPE_TPWSTRING ? m_oTPWString : s_oEmpty;
}

const TPWString& PPropertyValue::GetTPWString() const
{
	static const TPWString s_oEmpty;
	return m_pType == TYPE_TPWSTRING ? m_oTPWString : s_oEmpty;
}

TLString& PPropertyValue::GetTLString()
{
	static TLString s_oEmpty;
	return m_pType == TYPE_TLSTRING ? m_oTLString : s_oEmpty;
}

const TLString& PPropertyValue::GetTLString() const
{
	static const TLString s_oEmpty;
	return m_pType == TYPE_TLSTRING ? m_oTLString : s_oEmpty;
}

// The type is not checked, as in the original.
const PPropertyName& PPropertyValue::GetPropertyName() const
{
	return *m_pName;
}

PProperties* PPropertyValue::GetProperties() const
{
	return m_pType == TYPE_PROPS ? m_pProperties.Get() : nullptr;
}

// The type is not checked, as in the original.
PPropertyValueArray* PPropertyValue::GetArray() const
{
	return m_pArray.Get();
}

TObject* PPropertyValue::GetTObject() const
{
	return m_pType != TYPE_UNDEF ? m_pObject : nullptr;
}

TManagedPtr<PProperties>& PPropertyValue::GetPropertiesMP()
{
	return m_pProperties;
}

TManagedPtr<PPropertyValueArray>& PPropertyValue::GetPropArrayMP()
{
	return m_pArray;
}

// --- Setters --------------------------------------------------------------------------------

void PPropertyValue::SetInteger(int value)
{
	ChangeType(TYPE_INT);
	m_iValue = value;
}

void PPropertyValue::SetUINT32(unsigned int value)
{
	ChangeType(TYPE_UINT32);
	m_uiValue = value;
}

void PPropertyValue::SetFloat(float value)
{
	ChangeType(TYPE_FLOAT);
	m_fValue = value;
}

void PPropertyValue::SetBoolean(bool value)
{
	ChangeType(TYPE_BOOL);
	m_bValue = value;
}

void PPropertyValue::SetTPCString(const TPCString& value)
{
	ChangeType(TYPE_TPCSTRING);
	m_oTPCString = value;
}

void PPropertyValue::SetTPWString(const TPWString& value)
{
	ChangeType(TYPE_TPWSTRING);
	m_oTPWString = value;
}

void PPropertyValue::SetTLString(const TLString& value)
{
	ChangeType(TYPE_TLSTRING);
	m_oTLString = value;
}

// Quirk: setting a name on a name leaks the old copy, as in the original.
void PPropertyValue::SetPropertyName(const PPropertyName& value)
{
	ChangeType(TYPE_PROPNAME);
	m_pName = new PPropertyName(value);
}

void PPropertyValue::SetProperties(PProperties* value)
{
	ChangeType(TYPE_PROPS);
	m_pProperties = value;
}

void PPropertyValue::SetArray(PPropertyValueArray* value)
{
	ChangeType(TYPE_ARRAY);
	m_pArray = value;
}

void PPropertyValue::SetTObject(TObject* value)
{
	ChangeType(value == nullptr ? TYPE_UNDEF : &value->GetClass());
	m_pObject = value;
}

void PPropertyValue::SetUndef()
{
	ChangeType(TYPE_UNDEF);
}

const char* TSTDCALL PPropertyValue::ToString(const TClass* type)
{
	return type != TYPE_UNDEF ? type->GetName() : "TNULL";
}

// --- PPropertyValueArray --------------------------------------------------------------------

PPropertyValueArray::PPropertyValueArray() : m_oValues(0, 0)
{
}

PPropertyValueArray::PPropertyValueArray(int count) : m_oValues(count, count)
{
}

PPropertyValueArray::PPropertyValueArray(PPropertyValue* values, int count) : m_oValues(1, count)
{
	for (int i = 0; i < count; ++i)
		m_oValues.Data()[i] = values[i];
}

PPropertyValueArray::PPropertyValueArray(const PPropertyValueArray& other) : TRefCounted(), m_oValues(other.m_oValues)
{
}

PPropertyValueArray::~PPropertyValueArray()
{
}

void PPropertyValueArray::Delete()
{
	delete this;
}

bool PPropertyValueArray::CanBeTypeArray(const TClass* type, int count) const
{
	const int size = m_oValues.GetNumElements();
	if (count != -1 && size != count)
		return false;
	for (int i = 0; i < size; ++i)
		if (!m_oValues[i].CanBeType(type))
			return false;
	return true;
}

bool PPropertyValueArray::GetTVector2(TVector2& vector) const
{
	if (!CanBeTypeArray(PPropertyValue::TYPE_FLOAT, 2))
		return false;
	vector.x = ToFloat(m_oValues[0]);
	vector.y = ToFloat(m_oValues[1]);
	return true;
}

bool PPropertyValueArray::GetTVector3(TVector3& vector) const
{
	if (!CanBeTypeArray(PPropertyValue::TYPE_FLOAT, 3))
		return false;
	vector.x = ToFloat(m_oValues[0]);
	vector.y = ToFloat(m_oValues[1]);
	vector.z = ToFloat(m_oValues[2]);
	return true;
}

bool PPropertyValueArray::GetTQuaternion(TQuaternion& quaternion) const
{
	if (!CanBeTypeArray(PPropertyValue::TYPE_FLOAT, 4))
		return false;
	quaternion.x = ToFloat(m_oValues[0]);
	quaternion.y = ToFloat(m_oValues[1]);
	quaternion.z = ToFloat(m_oValues[2]);
	quaternion.w = ToFloat(m_oValues[3]);
	return true;
}
