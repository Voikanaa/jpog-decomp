#pragma once

#include "common/abi.h"

namespace Toshi
{
class TPCString;
class TVector3;
} // namespace Toshi
class PProperties;

// A unit type's attributes (SimJP.exe). So far only the static getters AConstants uses: true,
// with `value` set, when `properties` has `name` as a value that can be the type.
class AUnitFactoryUnitType
{
public:
	static bool TSTDCALL GetUnitAttribute(int& value, const Toshi::TPCString& name, const PProperties* properties);
	static bool TSTDCALL GetUnitAttribute(float& value, const Toshi::TPCString& name, const PProperties* properties);
	static bool TSTDCALL GetUnitAttribute(Toshi::TVector3& value, const Toshi::TPCString& name,
	                                      const PProperties* properties);
};
