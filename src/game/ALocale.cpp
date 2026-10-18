#include "game/ALocale.h"

namespace ALocale
{

namespace
{

MeasurementSystem s_eMeasurementSystem = MeasurementSystem_Metric;

} // namespace

MeasurementSystem __stdcall GetMeasurementSystem()
{
	return s_eMeasurementSystem;
}

void __stdcall SetMeasurementSystem(MeasurementSystem system)
{
	s_eMeasurementSystem = system;
}

} // namespace ALocale
