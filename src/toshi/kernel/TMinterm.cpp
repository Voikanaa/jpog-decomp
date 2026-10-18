#include "toshi/kernel/TMinterm.h"

#include "toshi/kernel/TCString.h"
#include "toshi/kernel/TCStringPool.h"
#include "toshi/kernel/TSystem.h"

namespace Toshi
{

namespace
{

enum OpName
{
	OP_TRUE,
	OP_FALSE,
	OP_AND,
	OP_OR,
	OP_NOT,
	OP_OPEN,
	OP_CLOSE,
};

} // namespace

const char* TMinterm::ms_aEnglishOpNames[7] = {"true", "false", " and ", " or ", "not ", "(", ")"};
const char* TMinterm::ms_aMathOpNames[7] = {"1", "0", " & ", " + ", "~", "(", ")"};

// --- TMinterm ------------------------------------------------------------------------------

TMinterm::TMinterm() : m_uiMask(0), m_uiValue(0)
{
}

TMinterm::TMinterm(bool truth) : m_uiMask(truth ? 0 : 0xFFFFFFFF), m_uiValue(0)
{
}

TMinterm::TMinterm(unsigned int mask, unsigned int value) : m_uiMask(mask), m_uiValue(value)
{
}

TMinterm::TMinterm(const TMinterm& other) = default;

TMinterm& TMinterm::operator=(const TMinterm& other) = default;

bool TMinterm::operator==(const TMinterm& other)
{
	return m_uiMask == other.m_uiMask && m_uiValue == other.m_uiValue;
}

bool TMinterm::GetTruth(unsigned int inputs)
{
	return (m_uiMask & inputs) == m_uiValue;
}

// Bits beyond the name count are left out; bit numbers wrap at 32, as in the original.
void TMinterm::MakeFunctionString(TCString& out, const char** const opNames, const TArray<TPCString>& names) const
{
	if (m_uiMask == 0)
	{
		out += opNames[OP_TRUE];
		return;
	}
	if (m_uiMask == 0xFFFFFFFF && m_uiValue == 0)
	{
		out += opNames[OP_FALSE];
		return;
	}
	bool first = true;
	for (int i = 0; i < names.GetNumElements(); ++i)
	{
		const unsigned int bit = 1u << (i & 31);
		if ((m_uiMask & bit) == 0)
			continue;
		if (!first)
			out += opNames[OP_AND];
		if ((m_uiValue & bit) == 0)
			out += opNames[OP_NOT];
		out += names[i].GetCString().GetString(0);
		first = false;
	}
}

TPCString TMinterm::MakeFunctionString(const char** const opNames, const TArray<TPCString>& names) const
{
	TCString text;
	MakeFunctionString(text, opNames, names);
	return TSystem::GetCStringPool()->Get(text);
}

// --- TSumOfMinterms ------------------------------------------------------------------------

TSumOfMinterms::TSumOfMinterms() : m_oMinterms(4, 1)
{
}

TSumOfMinterms::TSumOfMinterms(int size) : m_oMinterms(4, size)
{
}

TSumOfMinterms::TSumOfMinterms(const TMinterm& minterm) : m_oMinterms(4, 1)
{
	m_oMinterms.Push(minterm);
}

TSumOfMinterms::TSumOfMinterms(const TSumOfMinterms& other) = default;

TSumOfMinterms::~TSumOfMinterms()
{
}

TSumOfMinterms& TSumOfMinterms::operator=(const TSumOfMinterms& other) = default;

TSumOfMinterms& TSumOfMinterms::operator+(const TMinterm& minterm)
{
	m_oMinterms.Push(minterm);
	return *this;
}

bool TSumOfMinterms::GetTruth(unsigned int inputs)
{
	if (m_oMinterms.GetNumElements() == 0)
		return true;
	for (int i = 0; i < m_oMinterms.GetNumElements(); ++i)
		if (m_oMinterms[i].GetTruth(inputs))
			return true;
	return false;
}

// Several minterms are each bracketed and joined with "or".
void TSumOfMinterms::MakeFunctionString(TCString& out, const char** const opNames, const TArray<TPCString>& names) const
{
	const int count = m_oMinterms.GetNumElements();
	if (count == 0)
	{
		out += opNames[OP_TRUE];
		return;
	}
	if (count == 1)
	{
		m_oMinterms[0].MakeFunctionString(out, opNames, names);
		return;
	}
	for (int i = 0; i < count; ++i)
	{
		if (i != 0)
			out += opNames[OP_OR];
		out += opNames[OP_OPEN];
		m_oMinterms[i].MakeFunctionString(out, opNames, names);
		out += opNames[OP_CLOSE];
	}
}

TPCString TSumOfMinterms::MakeFunctionString(const char** const opNames, const TArray<TPCString>& names) const
{
	TCString text;
	MakeFunctionString(text, opNames, names);
	return TSystem::GetCStringPool()->Get(text);
}

bool TSumOfMinterms::ParseFunctionString(const TCString&, const char** const, const TArray<TPCString>&)
{
	return false;
}

} // namespace Toshi
