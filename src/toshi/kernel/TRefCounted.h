#pragma once

#include "common/layout.h"

namespace Toshi
{

// Intrusive reference count base (TKernelInterface.dll). Counting only: the owner decides what
// to do at zero. Copies start at zero; assignment keeps the count.
class TRefCounted
{
public:
	TRefCounted();
	~TRefCounted();

	// The argument is unused.
	int IncRefCount(void* owner);
	int DecRefCount(void* owner);
	int GetRefCount() const;

private:
	TRefCounted(const TRefCounted& other);
	TRefCounted& operator=(const TRefCounted& other);

	int m_iRefCount;
};

JPOG_ASSERT_SIZE(TRefCounted, 4);

} // namespace Toshi
