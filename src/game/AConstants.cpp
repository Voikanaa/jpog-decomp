#include "game/AConstants.h"

#include "game/AUnitFactoryUnitType.h"
#include "ppropertyparser/PProperties.h"
#include "ppropertyparser/PPropertyReader.h"
#include "ppropertyparser/PPropertyValue.h"
#include "toshi/kernel/TSystem.h"
#include "toshi/kernel/strings/TCString.h"

using namespace Toshi;

const char* AConstantTypes::sm_caszFloatConstantNames[FLOAT_CONSTANT_COUNT] = {
#define JPOG_FLOAT_CONSTANT(name) #name,
#include "game/AConstantNames.inl"
#undef JPOG_FLOAT_CONSTANT
};

const char* AConstantTypes::sm_caszIntConstantNames[INT_CONSTANT_COUNT] = {
#define JPOG_INT_CONSTANT(name) #name,
#include "game/AConstantNames.inl"
#undef JPOG_INT_CONSTANT
};

const char* AConstantTypes::sm_caszVector3ConstantNames[VECTOR3_CONSTANT_COUNT] = {
#define JPOG_VECTOR3_CONSTANT(name) #name,
#include "game/AConstantNames.inl"
#undef JPOG_VECTOR3_CONSTANT
};

float AConstants::m_aFloatConstants[AConstantTypes::FLOAT_CONSTANT_COUNT];
int AConstants::m_aIntConstants[AConstantTypes::INT_CONSTANT_COUNT];
TVector3 AConstants::m_aVector3Constants[AConstantTypes::VECTOR3_CONSTANT_COUNT];
bool AConstants::m_bInitialised;

namespace
{

// The group's properties. Deviation: null for a missing group, which the original dereferences.
const PProperties* Group(const PProperties& properties, const char* name)
{
	const PPropertyValue* group = properties.GetProperty(TSystem::GetCStringPool()->Get(name));
	return group != nullptr ? group->GetProperties() : nullptr;
}

template <typename T>
void Read(T* values, const char* const* names, int count, const PProperties* group, const T& initial)
{
	for (int i = 0; i < count; ++i)
	{
		values[i] = initial;
		if (group != nullptr)
			AUnitFactoryUnitType::GetUnitAttribute(values[i], TSystem::GetCStringPool()->Get(names[i]), group);
	}
}

} // namespace

void TSTDCALL AConstants::Initialise()
{
	PPropertyReader reader;
	PProperties properties;
	if (!reader.Load(TCString("Data/Constant.ini"), properties))
		return;
	Read(m_aFloatConstants, AConstantTypes::sm_caszFloatConstantNames, AConstantTypes::FLOAT_CONSTANT_COUNT,
	     Group(properties, "FloatConstants"), 0.0f);
	Read(m_aIntConstants, AConstantTypes::sm_caszIntConstantNames, AConstantTypes::INT_CONSTANT_COUNT,
	     Group(properties, "IntConstants"), 0);
	Read(m_aVector3Constants, AConstantTypes::sm_caszVector3ConstantNames, AConstantTypes::VECTOR3_CONSTANT_COUNT,
	     Group(properties, "Vector3Constants"), TVector3::VEC_ZERO);
	m_bInitialised = true;
}

float TSTDCALL AConstants::GetFloatConstant(AConstantTypes::eFloatConstants constant)
{
	return m_aFloatConstants[constant];
}

int TSTDCALL AConstants::GetIntConstant(AConstantTypes::eIntConstants constant)
{
	return m_aIntConstants[constant];
}

const TVector3& TSTDCALL AConstants::GetVector3Constant(AConstantTypes::eVector3Constants constant)
{
	return m_aVector3Constants[constant];
}
