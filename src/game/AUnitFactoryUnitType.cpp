#include "game/AUnitFactoryUnitType.h"

#include "ppropertyparser/PProperties.h"
#include "ppropertyparser/PPropertyValue.h"

using namespace Toshi;

bool TSTDCALL AUnitFactoryUnitType::GetUnitAttribute(int& value, const TPCString& name, const PProperties* properties)
{
	const PPropertyValue* property = properties->GetProperty(name);
	if (property == nullptr || !property->CanBeType(PPropertyValue::TYPE_INT))
		return false;
	value = property->GetInteger();
	return true;
}

bool TSTDCALL AUnitFactoryUnitType::GetUnitAttribute(float& value, const TPCString& name, const PProperties* properties)
{
	const PPropertyValue* property = properties->GetProperty(name);
	if (property == nullptr || !property->CanBeType(PPropertyValue::TYPE_FLOAT))
		return false;
	value = property->GetFloat();
	return true;
}

bool TSTDCALL AUnitFactoryUnitType::GetUnitAttribute(TVector3& value, const TPCString& name,
                                                     const PProperties* properties)
{
	const PPropertyValue* property = properties->GetProperty(name);
	if (property == nullptr || !property->CanBeType(PPropertyValue::TYPE_ARRAY))
		return false;
	return property->GetArray()->GetTVector3(value);
}
