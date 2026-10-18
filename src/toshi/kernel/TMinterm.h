#pragma once

#include "common/layout.h"
#include "toshi/kernel/TArray.h"

namespace Toshi
{

class TCString;
class TPCString;

// A product term over up to 32 boolean inputs (TKernelInterface.dll): true for inputs whose
// masked bits equal the value. Op name tables: true, false, and, or, not, "(", ")".
class TMinterm
{
public:
	TMinterm();
	// true: the empty product (always true); false: mask all ones, value zero.
	explicit TMinterm(bool truth);
	TMinterm(unsigned int mask, unsigned int value);
	TMinterm(const TMinterm& other);
	TMinterm& operator=(const TMinterm& other);

	bool operator==(const TMinterm& other);
	bool GetTruth(unsigned int inputs);

	// Appends the term using `opNames` and the input names (one per bit).
	void MakeFunctionString(TCString& out, const char** const opNames, const TArray<TPCString>& names) const;
	TPCString MakeFunctionString(const char** const opNames, const TArray<TPCString>& names) const;

	static const char* ms_aEnglishOpNames[7];
	static const char* ms_aMathOpNames[7];

private:
	unsigned int m_uiMask;
	unsigned int m_uiValue;
};

JPOG_ASSERT_SIZE(TMinterm, 8);

// A sum (or) of minterms; the empty sum is true.
class TSumOfMinterms
{
public:
	TSumOfMinterms();
	explicit TSumOfMinterms(int size);
	TSumOfMinterms(const TMinterm& minterm);
	TSumOfMinterms(const TSumOfMinterms& other);
	~TSumOfMinterms();
	TSumOfMinterms& operator=(const TSumOfMinterms& other);

	// Adds a minterm (an or).
	TSumOfMinterms& operator+(const TMinterm& minterm);
	bool GetTruth(unsigned int inputs);

	void MakeFunctionString(TCString& out, const char** const opNames, const TArray<TPCString>& names) const;
	TPCString MakeFunctionString(const char** const opNames, const TArray<TPCString>& names) const;
	// Not implemented in the shipped build: always fails.
	bool ParseFunctionString(const TCString& text, const char** const opNames, const TArray<TPCString>& names);

private:
	TArray<TMinterm> m_oMinterms;
};

JPOG_ASSERT_SIZE(TSumOfMinterms, 16);

} // namespace Toshi
