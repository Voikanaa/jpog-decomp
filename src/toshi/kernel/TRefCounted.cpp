#include "toshi/kernel/TRefCounted.h"

namespace Toshi
{

TRefCounted::TRefCounted() : m_iRefCount(0)
{
}

TRefCounted::TRefCounted(const TRefCounted&) : m_iRefCount(0)
{
}

TRefCounted::~TRefCounted()
{
}

TRefCounted& TRefCounted::operator=(const TRefCounted&)
{
	return *this;
}

int TRefCounted::IncRefCount(void*)
{
	return ++m_iRefCount;
}

int TRefCounted::DecRefCount(void*)
{
	return --m_iRefCount;
}

int TRefCounted::GetRefCount() const
{
	return m_iRefCount;
}

} // namespace Toshi
