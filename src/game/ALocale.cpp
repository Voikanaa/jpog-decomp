#include "game/ALocale.h"

namespace ALocale
{

namespace
{

MeasurementSystem s_eMeasurementSystem = MeasurementSystem_Metric;

} // namespace

MeasurementSystem TSTDCALL GetMeasurementSystem()
{
	return s_eMeasurementSystem;
}

void TSTDCALL SetMeasurementSystem(MeasurementSystem system)
{
	s_eMeasurementSystem = system;
}

} // namespace ALocale
