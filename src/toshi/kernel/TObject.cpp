#include "toshi/kernel/TObject.h"

#include "toshi/kernel/TSystem.h"
#include "toshi/kernel/strings/TCString.h"

#include <new>

namespace Toshi
{

TClass TObject::m_sClass("TObject", nullptr, &TObject::CreateObject, &TObject::CreateObjectInPlace,
                         &TObject::InitialiseStatic, &TObject::DeinitialiseStatic, 0x20000);

namespace
{

// Indentation depth while dumping the class tree.
int s_dumpDepth = 0;

bool TSTDCALL DumpEnter(TClass*, void*)
{
	++s_dumpDepth;
	return true;
}

bool TSTDCALL DumpLeave(TClass*, void*)
{
	--s_dumpDepth;
	return true;
}

bool TSTDCALL DumpClass(TClass* cls, void*)
{
	for (int i = 0; i < s_dumpDepth; ++i)
		TCString().Format("    ").Print();
	TCString().Format("%s : ver=%u.%u\n", cls->GetName(), cls->GetVersionMajor(), cls->GetVersionMinor()).Print();
	return true;
}

} // namespace

// --- TObject -------------------------------------------------------------------------------

TObject::TObject()
{
}

TObject::TObject(const TObject&)
{
}

TObject::~TObject()
{
}

TClass& TObject::GetClass() const
{
	return m_sClass;
}

void TObject::Delete()
{
	delete this;
}

bool TObject::IsA(const TClass& cls) const
{
	return GetClass().IsA(cls);
}

bool TObject::IsExactly(const TClass& cls) const
{
	return &GetClass() == &cls;
}

TObject* TSTDCALL TObject::CreateObject()
{
	return new TObject();
}

TObject* TSTDCALL TObject::CreateObjectInPlace(void* memory)
{
	return memory == nullptr ? nullptr : new (memory) TObject();
}

void TSTDCALL TObject::InitialiseStatic()
{
}

void TSTDCALL TObject::DeinitialiseStatic()
{
}

// --- TClass --------------------------------------------------------------------------------

TClass::TClass(const char* name, TClass* parent, CreateFunction create, CreateInPlaceFunction createInPlace,
               StaticFunction initialise, StaticFunction deinitialise, unsigned int version)
{
	m_szName = name;
	m_fnCreate = create;
	m_fnCreateInPlace = createInPlace;
	m_fnInitialise = initialise;
	m_fnDeinitialise = deinitialise;
	m_pParent = parent;
	m_uiVersion = version;
	m_bInitialised = false;
	AttachClassToParent();
}

TClass::~TClass()
{
	DetachClassFromParent();
	m_pFirstChild = nullptr;
	m_pNextSibling = nullptr;
	m_pParent = nullptr;
	if (m_bInitialised)
		DeinitialiseStatic();
}

TClass& TClass::operator=(const TClass& other)
{
	m_szName = other.m_szName;
	m_fnCreate = other.m_fnCreate;
	m_fnCreateInPlace = other.m_fnCreateInPlace;
	m_fnInitialise = other.m_fnInitialise;
	m_fnDeinitialise = other.m_fnDeinitialise;
	m_pParent = other.m_pParent;
	m_pNextSibling = other.m_pNextSibling;
	m_pFirstChild = other.m_pFirstChild;
	m_uiVersion = other.m_uiVersion;
	m_bInitialised = other.m_bInitialised;
	return *this;
}

TObject* TClass::CreateObject() const
{
	return m_fnCreate != nullptr ? m_fnCreate() : nullptr;
}

TObject* TClass::CreateObjectInPlace(void* memory) const
{
	return m_fnCreateInPlace != nullptr ? m_fnCreateInPlace(memory) : nullptr;
}

void TClass::InitialiseStatic()
{
	if (m_fnInitialise != nullptr)
	{
		m_fnInitialise();
		m_bInitialised = true;
	}
}

void TClass::DeinitialiseStatic()
{
	if (m_fnDeinitialise != nullptr)
	{
		m_bInitialised = false;
		m_fnDeinitialise();
	}
}

bool TClass::IsStaticInitialised() const
{
	return m_bInitialised;
}

bool TClass::AttachClassToParent()
{
	if (m_pParent == nullptr || IsAttached())
		return false;
	m_pNextSibling = m_pParent->m_pFirstChild;
	m_pParent->m_pFirstChild = this;
	return true;
}

bool TClass::DetachClassFromParent()
{
	return true;
}

bool TClass::IsAttached() const
{
	if (m_pParent != nullptr)
		for (const TClass* child = m_pParent->m_pFirstChild; child != nullptr; child = child->m_pNextSibling)
			if (child == this)
				return true;
	return false;
}

bool TClass::IsA(const TClass& cls) const
{
	for (const TClass* current = this; current != nullptr; current = current->m_pParent)
		if (current->IsExactly(cls))
			return true;
	return false;
}

bool TClass::IsExactly(const TClass& cls) const
{
	return this == &cls;
}

bool TClass::operator==(const TClass& other) const
{
	return m_szName == other.m_szName;
}

const char* TClass::GetName() const
{
	return m_szName;
}

const TClass* TClass::GetParent() const
{
	return m_pParent;
}

unsigned int TClass::GetVersion() const
{
	return m_uiVersion;
}

unsigned short TClass::GetVersionMajor() const
{
	return static_cast<unsigned short>(m_uiVersion >> 16);
}

unsigned short TClass::GetVersionMinor() const
{
	return static_cast<unsigned short>(m_uiVersion & 0xFF);
}

void TClass::RecurseTree(RecurseFunction enter, RecurseFunction leave, RecurseFunction each, void* userData)
{
	if (!each(this, userData))
		return;
	if (enter != nullptr)
		enter(this, userData);
	RecurseTree2(enter, leave, each, userData);
	if (leave != nullptr)
		leave(this, userData);
}

void TClass::RecurseTree2(RecurseFunction enter, RecurseFunction leave, RecurseFunction each, void* userData)
{
	for (TClass* child = m_pFirstChild; child != nullptr; child = child->m_pNextSibling)
	{
		if (each != nullptr)
			each(child, userData);
		if (child->m_pFirstChild != nullptr)
		{
			if (enter != nullptr)
				enter(this, userData);
			child->RecurseTree2(enter, leave, each, userData);
			if (leave != nullptr)
				leave(this, userData);
		}
	}
}

const TClass* TSTDCALL TClass::Find(const char* name, const TClass* root)
{
	return FindRecurse(name, root != nullptr ? root : &TObject::m_sClass, false);
}

const TClass* TSTDCALL TClass::FindRecurse(const char* name, const TClass* cls, bool siblings)
{
	while (cls != nullptr)
	{
		const TClass* next = siblings ? cls->m_pNextSibling : nullptr;
		if (TSystem::StringCompareNoCase(cls->m_szName, name, -1) == 0)
			return cls;
		if (cls->m_pFirstChild != nullptr)
		{
			if (const TClass* found = FindRecurse(name, cls->m_pFirstChild, true))
				return found;
		}
		cls = next;
	}
	return nullptr;
}

// Walks both ancestor chains from the root down and returns the last class they share.
const TClass* TSTDCALL TClass::FindCommonBaseClass(const TClass& a, const TClass& b)
{
	if (&a == &b)
		return &a;
	const TClass* chainA[64];
	const TClass* chainB[64];
	int lengthA = 0;
	int lengthB = 0;
	for (const TClass* cls = &a; cls != nullptr; cls = cls->m_pParent)
		chainA[lengthA++] = cls;
	for (const TClass* cls = &b; cls != nullptr; cls = cls->m_pParent)
		chainB[lengthB++] = cls;

	const TClass* common = nullptr;
	if (lengthA == 0)
		return common;
	while (lengthB != 0)
	{
		const TClass* fromA = chainA[lengthA - 1];
		if (fromA != chainB[lengthB - 1])
			return common;
		--lengthA;
		--lengthB;
		if (lengthA == 0)
			return fromA;
		common = fromA;
	}
	return common;
}

void TSTDCALL TClass::DumpObjectClassTree()
{
	s_dumpDepth = 0;
	auto* root = const_cast<TClass*>(FindRecurse("TObject", &TObject::m_sClass, false));
	root->RecurseTree(&DumpEnter, &DumpLeave, &DumpClass, nullptr);
}

} // namespace Toshi
