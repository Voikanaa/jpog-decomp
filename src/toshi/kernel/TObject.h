#pragma once

namespace Toshi
{

class TClass;

// Root of the Toshi RTTI hierarchy (TKernelInterface.dll). The virtual order matches the
// original vtable (symbols/vtables.csv): GetClass, Delete, destructor.
// Declaration only so far; implemented by the original DLL in the x86 build.
class TObject
{
public:
	virtual TClass& GetClass() const;
	virtual void Delete();
	virtual ~TObject();
};

// Runtime class descriptor (TKernelInterface.dll). Declaration only so far.
class TClass
{
public:
	// Finds a registered class by name, optionally starting the search at `parent`.
	static const TClass* __stdcall Find(const char* name, const TClass* parent);
	TObject* CreateObject() const;
};

} // namespace Toshi
