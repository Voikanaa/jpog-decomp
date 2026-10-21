#pragma once

#include "common/abi.h"
#include "common/layout.h"
#include "ppropertyparser/PPropertyName.h"
#include "ppropertyparser/PPropertyValue.h"
#include "toshi/kernel/TObject.h"
#include "toshi/kernel/TRefCounted.h"
#include "toshi/kernel/containers/TQList.h"

#include <cstddef>

namespace Toshi
{
class TFreeList;
}

// A set of named properties in file order (PPropertyParser.dll). Sets nest through property
// values; each keeps a pointer to its parent. Reference-counted (TManagedPtr) and allocated from
// a free list, like its properties.
class PProperties : public Toshi::TObject, public Toshi::TRefCounted
{
public:
	// One name/value pair with an optional comment.
	class PProperty : public Toshi::TQList<PProperty>::TNode
	{
	public:
		PProperty(const PPropertyName& name, const PPropertyValue& value);
		PProperty(const PPropertyName& name, const PPropertyValue& value, const Toshi::TPCString& comment);
		// Quirk: copies the list links too, as in the original.
		PProperty(const PProperty& other);
		~PProperty();
		PProperty& operator=(const PProperty& other);

		const PPropertyName& GetName() const;
		const PPropertyValue* GetValue() const;
		void SetValue(const PPropertyValue& value);
		Toshi::TPCString GetComment() const;
		void SetComment(const Toshi::TPCString& comment);

		static void* TSTDCALL operator new(std::size_t size);
		static void* TSTDCALL operator new(std::size_t size, char* file, int line);
		static void* TSTDCALL operator new(std::size_t size, void* place);
		static void TSTDCALL operator delete(void* memory);
		static Toshi::TFreeList& TSTDCALL GetFreeList();

	private:
		friend class PProperties;

		static Toshi::TFreeList ms_oFreelist;

		PPropertyName m_oName;
		PPropertyValue m_oValue;
		Toshi::TPCString m_oComment;
	};

	PProperties();
	explicit PProperties(PProperties* parent);
	~PProperties() override;

	Toshi::TClass& GetClass() const override;

	Toshi::TQList<PProperty>::Iterator Begin() const;
	Toshi::TQList<PProperty>::Iterator End() const;
	Toshi::TQList<PProperty>& GetPropertySet();
	const Toshi::TQList<PProperty>& GetPropertySet() const;
	int GetPropertyCount() const;
	bool IsEmpty() const;
	PProperties* GetParentProperties() const;

	// Appends a property (duplicates allowed).
	void PutProperty(const PPropertyName& name, const PPropertyValue& value);
	void PutProperty(const PPropertyName& name, const PPropertyValue& value, const Toshi::TPCString& comment);
	void PutProperty(const Toshi::TPCString& name, const PPropertyValue& value);
	void PutProperty(const Toshi::TPCString& name, const PPropertyValue& value, const Toshi::TPCString& comment);
	void PutProperty(const Toshi::TPCString& name, const Toshi::TPCString& subName, const PPropertyValue& value);
	void PutProperty(const Toshi::TPCString& name, const Toshi::TPCString& subName, const PPropertyValue& value,
	                 const Toshi::TPCString& comment);
	// Replaces the value of the first property with the name, or appends one. Quirk: the
	// variants with a comment always append, as in the original.
	void PutPropertyUnique(const PPropertyName& name, const PPropertyValue& value);
	void PutPropertyUnique(const PPropertyName& name, const PPropertyValue& value, const Toshi::TPCString& comment);
	void PutPropertyUnique(const Toshi::TPCString& name, const PPropertyValue& value);
	void PutPropertyUnique(const Toshi::TPCString& name, const PPropertyValue& value, const Toshi::TPCString& comment);
	void PutPropertyUnique(const Toshi::TPCString& name, const Toshi::TPCString& subName, const PPropertyValue& value);
	void PutPropertyUnique(const Toshi::TPCString& name, const Toshi::TPCString& subName, const PPropertyValue& value,
	                       const Toshi::TPCString& comment);

	// The value of the first property with the name; a plain string name is split ("a[b]").
	const PPropertyValue* GetProperty(const PPropertyName& name) const;
	const PPropertyValue* GetProperty(const Toshi::TPCString& name) const;
	const PPropertyValue* GetProperty(const Toshi::TPCString& name, const Toshi::TPCString& subName) const;
	// The sub-name is the index in decimal ("name[3]").
	const PPropertyValue* GetProperty(const Toshi::TPCString& name, int index) const;
	// GetProperty, but null unless the value is exactly of `type`.
	const PPropertyValue* GetRequiredProperty(const Toshi::TPCString& name, const Toshi::TClass* type) const;
	const PPropertyValue* GetRequiredProperty(const Toshi::TPCString& name, const Toshi::TPCString& subName,
	                                          const Toshi::TClass* type) const;
	const PPropertyValue* GetRequiredProperty(const Toshi::TPCString& name, int index, const Toshi::TClass* type) const;
	const PPropertyValue* GetOptionalProperty(const Toshi::TPCString& name, const Toshi::TClass* type) const;
	const PPropertyValue* GetOptionalProperty(const Toshi::TPCString& name, const Toshi::TPCString& subName,
	                                          const Toshi::TClass* type) const;
	const PPropertyValue* GetOptionalProperty(const Toshi::TPCString& name, int index, const Toshi::TClass* type) const;
	// The next property from `position` whose name (not sub-name) is the same pooled string;
	// `position` moves past it, or to End() when there is none.
	const PProperty* FindProperty(const Toshi::TPCString& name, Toshi::TQList<PProperty>::Iterator& position) const;
	// Orders the properties by name.
	void Sort();

	static void* TSTDCALL operator new(std::size_t size);
	static void* TSTDCALL operator new(std::size_t size, char* file, int line);
	static void* TSTDCALL operator new(std::size_t size, void* place);
	static void TSTDCALL operator delete(void* memory);
	static Toshi::TFreeList& TSTDCALL GetFreeList();

	static Toshi::TClass m_sClass;

private:
	static Toshi::TObject* TSTDCALL CreateObject();
	static Toshi::TObject* TSTDCALL CreateObjectInPlace(void* memory);
	static void TSTDCALL InitialiseStatic();
	static void TSTDCALL DeinitialiseStatic();

	static Toshi::TFreeList ms_oFreelist;

	PProperties* m_pParent;
	Toshi::TQList<PProperty> m_oProperties;
};

JPOG_ASSERT_SIZE(PProperties::PProperty, 0x1C);
JPOG_ASSERT_SIZE(PProperties, 0x18);
