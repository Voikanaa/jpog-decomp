#include "ppropertyparser/PPropertyBlock.h"

using namespace Toshi;

TClass PPropertyBlock::m_sClass("PPropertyBlock", &TObject::m_sClass, CreateObject, CreateObjectInPlace,
                                InitialiseStatic, DeinitialiseStatic, 0x10000);

PPropertyBlock::PPropertyBlock(const TPCString& name, Priority priority) : m_oName(name), m_ePriority(priority)
{
}

PPropertyBlock::~PPropertyBlock()
{
}

TClass& PPropertyBlock::GetClass() const
{
	return m_sClass;
}

const TPCString& PPropertyBlock::GetName() const
{
	return m_oName;
}

void PPropertyBlock::SetName(const TPCString& name)
{
	m_oName = name;
}

PPropertyBlock::Priority PPropertyBlock::GetPriority() const
{
	return m_ePriority;
}

TObject* TSTDCALL PPropertyBlock::CreateObject()
{
	return nullptr;
}

TObject* TSTDCALL PPropertyBlock::CreateObjectInPlace(void*)
{
	return nullptr;
}

void TSTDCALL PPropertyBlock::InitialiseStatic()
{
}

void TSTDCALL PPropertyBlock::DeinitialiseStatic()
{
}
