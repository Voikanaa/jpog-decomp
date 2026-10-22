#pragma once

#include "common/abi.h"

// Locale-dependent formatting (SimJP.exe). Only the measurement system so far.
namespace ALocale
{

enum MeasurementSystem
{
	MeasurementSystem_Metric,
	MeasurementSystem_Imperial,
};

MeasurementSystem TSTDCALL GetMeasurementSystem();
void TSTDCALL SetMeasurementSystem(MeasurementSystem system);

} // namespace ALocale
