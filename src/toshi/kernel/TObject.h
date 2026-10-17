#pragma once

#include "common/abi.h"
#include "common/layout.h"

namespace Toshi
{

class TClass;

// Root of the Toshi RTTI hierarchy (TKernelInterface.dll). The virtual order matches the
// original vtable (symbols/vtables.csv): GetClass, Delete, destructor.
class TObject
{
public:
	TObject(const TObject& other);

	virtual TClass& GetClass() const;
	// Destroys and frees the object (via operator delete, i.e. tfree).
	virtual void Delete();

	bool IsA(const TClass& cls) const;
	bool IsExactly(const TClass& cls) const;

	static TClass m_sClass;

protected:
	TObject();
	virtual ~TObject();

private:
	static TObject* TSTDCALL CreateObject();
	static TObject* TSTDCALL CreateObjectInPlace(void* memory);
	static void TSTDCALL InitialiseStatic();
	static void TSTDCALL DeinitialiseStatic();
};

JPOG_ASSERT_SIZE(TObject, 4);

// Runtime class descriptor (TKernelInterface.dll). Classes form a tree: each registers with its
// parent at construction (a static TClass per class), keeping a child list via sibling links.
class TClass
{
public:
	using CreateFunction = TObject*(TSTDCALL*)();
	using CreateInPlaceFunction = TObject*(TSTDCALL*)(void* memory);
	using StaticFunction = void(TSTDCALL*)();
	// Tree walk callback; the walk only uses the return value of the first `each` call.
	using RecurseFunction = bool(TSTDCALL*)(TClass* cls, void* userData);

	// The child links are deliberately not initialised: static TClass objects are zeroed before
	// construction, and children may attach before their parent's constructor has run.
	TClass(const char* name, TClass* parent, CreateFunction create, CreateInPlaceFunction createInPlace,
	       StaticFunction initialise, StaticFunction deinitialise, unsigned int version);
	~TClass();
	TClass& operator=(const TClass& other);

	TObject* CreateObject() const;
	TObject* CreateObjectInPlace(void* memory) const;
	void InitialiseStatic();
	void DeinitialiseStatic();
	bool IsStaticInitialised() const;

	bool AttachClassToParent();
	// Does nothing in the original (returns true).
	bool DetachClassFromParent();
	bool IsAttached() const;
	bool IsA(const TClass& cls) const;
	bool IsExactly(const TClass& cls) const;
	// Compares names by pointer, as the original does.
	bool operator==(const TClass& other) const;

	const char* GetName() const;
	const TClass* GetParent() const;
	unsigned int GetVersion() const;
	unsigned short GetVersionMajor() const;
	// Only the low byte of the version word, as in the original.
	unsigned short GetVersionMinor() const;

	// Calls `each(this)`; if it returns true, walks the subtree depth first, calling `each` for
	// every class and `enter`/`leave` (with the parent) around each child that has children.
	// `each` must not be null (the original calls it unchecked).
	void RecurseTree(RecurseFunction enter, RecurseFunction leave, RecurseFunction each, void* userData);

	// Case-insensitive search by name in the subtree of `root` (TObject's class if null).
	static const TClass* TSTDCALL Find(const char* name, const TClass* root);
	static const TClass* TSTDCALL FindCommonBaseClass(const TClass& a, const TClass& b);
	// Formats the class tree for debug output (TCString::Print, empty in the shipped build).
	static void TSTDCALL DumpObjectClassTree();

private:
	void RecurseTree2(RecurseFunction enter, RecurseFunction leave, RecurseFunction each, void* userData);
	static const TClass* TSTDCALL FindRecurse(const char* name, const TClass* cls, bool siblings);

	const char* m_szName;
	CreateFunction m_fnCreate;
	CreateInPlaceFunction m_fnCreateInPlace;
	StaticFunction m_fnInitialise;
	StaticFunction m_fnDeinitialise;
	TClass* m_pParent;
	TClass* m_pNextSibling;
	TClass* m_pFirstChild;
	unsigned int m_uiVersion;
	bool m_bInitialised;
};

JPOG_ASSERT_SIZE(TClass, 0x28);

} // namespace Toshi
