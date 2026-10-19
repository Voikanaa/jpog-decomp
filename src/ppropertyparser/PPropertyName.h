#pragma once

#include "common/abi.h"
#include "common/layout.h"
#include "toshi/kernel/TCStringPool.h"

// A property name with an optional sub-name, written "name[sub]" (PPropertyParser.dll).
class PPropertyName
{
public:
	PPropertyName();
	// Splits "name[sub]"; without a '[' the whole string is the name.
	explicit PPropertyName(const Toshi::TPCString& name);
	PPropertyName(const Toshi::TPCString& name, const Toshi::TPCString& subName);
	PPropertyName(const PPropertyName& other);
	~PPropertyName();
	PPropertyName& operator=(const PPropertyName& other);

	const Toshi::TPCString& GetName() const;
	const Toshi::TPCString& GetSubName() const;
	void SetName(const Toshi::TPCString& name);
	void SetSubName(const Toshi::TPCString& subName);
	// "name", or "name[sub]" when there is a sub-name.
	Toshi::TPCString GetString() const;

	// Ordered by name, then sub-name (string content).
	bool operator==(const PPropertyName& other) const;
	bool operator!=(const PPropertyName& other) const;
	bool operator<(const PPropertyName& other) const;
	bool operator>(const PPropertyName& other) const;

protected:
	// The name up to the first '[', and the sub-name up to the next ']' (if there is one). The
	// sub-name is left unchanged when there is no '['.
	void Split(const Toshi::TPCString& string);

private:
	Toshi::TPCString m_oName;
	Toshi::TPCString m_oSubName;
};

JPOG_ASSERT_SIZE(PPropertyName, 8);

// `string` followed by "name" or "name[sub]".
Toshi::TCString TSTDCALL operator+(const Toshi::TCString& string, const PPropertyName& name);
