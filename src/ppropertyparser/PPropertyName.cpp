#include "ppropertyparser/PPropertyName.h"

#include "toshi/kernel/TCString.h"
#include "toshi/kernel/TSystem.h"
#include "toshi/kernel/TWString.h"

using namespace Toshi;

namespace
{

const unsigned short kOpenBracket[] = {'[', 0};
const unsigned short kCloseBracket[] = {']', 0};

} // namespace

PPropertyName::PPropertyName()
{
}

PPropertyName::PPropertyName(const TPCString& name)
{
	Split(name);
}

PPropertyName::PPropertyName(const TPCString& name, const TPCString& subName) : m_oName(name), m_oSubName(subName)
{
}

PPropertyName::PPropertyName(const PPropertyName& other) = default;

PPropertyName::~PPropertyName()
{
}

PPropertyName& PPropertyName::operator=(const PPropertyName& other)
{
	m_oName = other.m_oName;
	m_oSubName = other.m_oSubName;
	return *this;
}

const TPCString& PPropertyName::GetName() const
{
	return m_oName;
}

const TPCString& PPropertyName::GetSubName() const
{
	return m_oSubName;
}

void PPropertyName::SetName(const TPCString& name)
{
	m_oName = name;
}

void PPropertyName::SetSubName(const TPCString& subName)
{
	m_oSubName = subName;
}

// Built through a wide string, as in the original.
TPCString PPropertyName::GetString() const
{
	TWString text(m_oName.GetCString().GetString(0));
	if (!m_oSubName.GetCString().IsEmpty())
		text += TWString((TCString(kOpenBracket) + m_oSubName) + kCloseBracket);
	return TSystem::GetCStringPool()->Get(TCString(text));
}

bool PPropertyName::operator==(const PPropertyName& other) const
{
	return m_oName.Compare(other.m_oName) == 0 && m_oSubName.Compare(other.m_oSubName) == 0;
}

bool PPropertyName::operator!=(const PPropertyName& other) const
{
	return m_oName.Compare(other.m_oName) != 0 || m_oSubName.Compare(other.m_oSubName) != 0;
}

bool PPropertyName::operator<(const PPropertyName& other) const
{
	const int order = m_oName.Compare(other.m_oName);
	if (order != 0)
		return order < 0;
	return m_oSubName.Compare(other.m_oSubName) < 0;
}

bool PPropertyName::operator>(const PPropertyName& other) const
{
	const int order = m_oName.Compare(other.m_oName);
	if (order != 0)
		return order > 0;
	return m_oSubName.Compare(other.m_oSubName) > 0;
}

void PPropertyName::Split(const TPCString& string)
{
	const TCString& text = string.GetCString();
	const int open = text.Find('[', 0);
	if (open == -1)
	{
		m_oName = string;
		return;
	}
	m_oName = TSystem::GetCStringPool()->Get(text.Mid(0, open));
	const int close = text.Find(']', open);
	if (close != -1)
		m_oSubName = TSystem::GetCStringPool()->Get(text.Mid(open + 1, close - open - 1));
}

TCString TSTDCALL operator+(const TCString& string, const PPropertyName& name)
{
	TCString result(string);
	result += name.GetName().GetCString().GetString(0);
	const TCString& subName = name.GetSubName().GetCString();
	if (!subName.IsEmpty())
		result += TCString("[") + subName.GetString(0) + "]";
	return result;
}
