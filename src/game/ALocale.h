#pragma once

// Locale-dependent formatting (SimJP.exe). Only the measurement system so far.
namespace ALocale
{

enum MeasurementSystem
{
	MeasurementSystem_Metric,
	MeasurementSystem_Imperial,
};

MeasurementSystem __stdcall GetMeasurementSystem();
void __stdcall SetMeasurementSystem(MeasurementSystem system);

} // namespace ALocale
