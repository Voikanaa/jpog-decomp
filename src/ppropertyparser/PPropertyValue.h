#pragma once

#include "common/abi.h"
#include "common/layout.h"
#include "toshi/kernel/TLocale.h"
#include "toshi/kernel/TRefCounted.h"
#include "toshi/kernel/containers/TArray.h"
#include "toshi/kernel/containers/TManagedPtr.h"
#include "toshi/kernel/strings/TCStringPool.h"
#include "toshi/kernel/strings/TWStringPool.h"

namespace Toshi
{
class TObject;
class TClass;
class TQuaternion;
class TVector2;
class TVector3;
} // namespace Toshi

class PProperties;
class PPropertyName;
class PPropertyValueArray;

// A typed property value (PPropertyParser.dll). The type is a TClass pointer: one of the TYPE_*
// tags, PProperties::m_sClass for nested property sets, an object's own class for TObject
// values, or null (TYPE_UNDEF). Strings, property sets and arrays are reference-counted.
class PPropertyValue
{
public:
	PPropertyValue();
	PPropertyValue(const PPropertyValue& other);
	explicit PPropertyValue(int value);
	explicit PPropertyValue(unsigned int value);
	explicit PPropertyValue(float value);
	explicit PPropertyValue(bool value);
	explicit PPropertyValue(const Toshi::TPCString& value);
	explicit PPropertyValue(const Toshi::TPWString& value);
	explicit PPropertyValue(const Toshi::TLString& value);
	explicit PPropertyValue(const PPropertyName& value);
	explicit PPropertyValue(PProperties* value);
	explicit PPropertyValue(PPropertyValueArray* value);
	explicit PPropertyValue(Toshi::TObject* value);
	~PPropertyValue();

	PPropertyValue& operator=(const PPropertyValue& other);
	PPropertyValue& operator=(int value);
	PPropertyValue& operator=(unsigned int value);
	PPropertyValue& operator=(float value);
	PPropertyValue& operator=(const Toshi::TPCString& value);
	PPropertyValue& operator=(const Toshi::TPWString& value);
	PPropertyValue& operator=(const Toshi::TLString& value);
	PPropertyValue& operator=(const PPropertyName& value);
	PPropertyValue& operator=(PProperties* value);
	PPropertyValue& operator=(PPropertyValueArray* value);

	// Strings, sets and arrays compare by handle (identity); a name by content.
	bool operator==(const PPropertyValue& other) const;
	bool operator!=(const PPropertyValue& other) const;
	bool operator==(int value) const;
	bool operator==(unsigned int value) const;
	bool operator==(float value) const;
	bool operator==(bool value) const;
	bool operator==(const Toshi::TPCString& value) const;
	bool operator==(const Toshi::TPWString& value) const;
	bool operator==(const Toshi::TLString& value) const;
	bool operator==(const char* value) const;
	bool operator==(const unsigned short* value) const;
	bool operator==(const PPropertyName& value) const;
	bool operator==(const PProperties* value) const;
	bool operator==(const PPropertyValueArray* value) const;
	bool operator==(const Toshi::TObject* value) const;

	const Toshi::TClass* GetType() const;
	bool IsType(const Toshi::TClass* type) const;
	bool IsDefined() const;
	bool IsEmpty() const;
	// Whether the value converts to `type` without loss: numbers between each other (ints to
	// float only below 1e8, floats to int only when integral), bools to numbers.
	bool CanBeType(const Toshi::TClass* type) const;

	// Numeric getters convert between the number types; other types give 0 / false.
	int GetInteger() const;
	unsigned int GetUINT32() const;
	float GetFloat() const;
	bool GetBoolean() const;
	// The string getters return a shared empty handle for other types.
	Toshi::TPCString& GetTPCString();
	const Toshi::TPCString& GetTPCString() const;
	Toshi::TPWString& GetTPWString();
	const Toshi::TPWString& GetTPWString() const;
	Toshi::TLString& GetTLString();
	const Toshi::TLString& GetTLString() const;
	const PPropertyName& GetPropertyName() const;
	PProperties* GetProperties() const;
	PPropertyValueArray* GetArray() const;
	Toshi::TObject* GetTObject() const;

	void SetInteger(int value);
	void SetUINT32(unsigned int value);
	void SetFloat(float value);
	void SetBoolean(bool value);
	void SetTPCString(const Toshi::TPCString& value);
	void SetTPWString(const Toshi::TPWString& value);
	void SetTLString(const Toshi::TLString& value);
	void SetPropertyName(const PPropertyName& value);
	void SetProperties(PProperties* value);
	void SetArray(PPropertyValueArray* value);
	void SetTObject(Toshi::TObject* value);
	void SetUndef();

	// The type's class name, "TNULL" for undefined.
	static const char* TSTDCALL ToString(const Toshi::TClass* type);

	PIMPORT_DATA static const Toshi::TClass* const TYPE_UNDEF;
	PIMPORT_DATA static const Toshi::TClass* const TYPE_INT;
	PIMPORT_DATA static const Toshi::TClass* const TYPE_UINT32;
	PIMPORT_DATA static const Toshi::TClass* const TYPE_FLOAT;
	PIMPORT_DATA static const Toshi::TClass* const TYPE_BOOL;
	PIMPORT_DATA static const Toshi::TClass* const TYPE_TPCSTRING;
	PIMPORT_DATA static const Toshi::TClass* const TYPE_TPWSTRING;
	PIMPORT_DATA static const Toshi::TClass* const TYPE_TLSTRING;
	PIMPORT_DATA static const Toshi::TClass* const TYPE_PROPS;
	PIMPORT_DATA static const Toshi::TClass* const TYPE_PROPNAME;
	PIMPORT_DATA static const Toshi::TClass* const TYPE_ARRAY;

protected:
	// Copies `other`'s type and value.
	void Assign(const PPropertyValue& other);
	// Releases the current value and switches to `type` (with an empty value); false if the
	// type does not change.
	bool ChangeType(const Toshi::TClass* type);
	Toshi::TManagedPtr<PProperties>& GetPropertiesMP();
	Toshi::TManagedPtr<PPropertyValueArray>& GetPropArrayMP();

private:
	const Toshi::TClass* m_pType;
	union
	{
		int m_iValue;
		unsigned int m_uiValue;
		float m_fValue;
		bool m_bValue;
		Toshi::TPCString m_oTPCString;
		Toshi::TPWString m_oTPWString;
		Toshi::TLString m_oTLString;
		Toshi::TManagedPtr<PProperties> m_pProperties;
		Toshi::TManagedPtr<PPropertyValueArray> m_pArray;
		PPropertyName* m_pName;
		Toshi::TObject* m_pObject;
	};
};

JPOG_ASSERT_SIZE(PPropertyValue, 8);

// A reference-counted array of values (PPropertyParser.dll).
class PPropertyValueArray : public Toshi::TRefCounted
{
public:
	PPropertyValueArray();
	// `count` undefined values (also the grow size).
	explicit PPropertyValueArray(int count);
	// Copies `count` values. Quirk: the element count stays 0, so the copies only become
	// visible through later pushes.
	PPropertyValueArray(PPropertyValue* values, int count);
	PPropertyValueArray(const PPropertyValueArray& other);
	~PPropertyValueArray();
	// Destroys and frees the array.
	void Delete();

	// Whether there are `count` elements (any number for -1) and each can be `type`.
	bool CanBeTypeArray(const Toshi::TClass* type, int count) const;
	bool GetTVector2(Toshi::TVector2& vector) const;
	bool GetTVector3(Toshi::TVector3& vector) const;
	bool GetTQuaternion(Toshi::TQuaternion& quaternion) const;

	Toshi::TArray<PPropertyValue>& GetValues() { return m_oValues; }
	const Toshi::TArray<PPropertyValue>& GetValues() const { return m_oValues; }

private:
	Toshi::TArray<PPropertyValue> m_oValues;
};

JPOG_ASSERT_SIZE(PPropertyValueArray, 0x14);
