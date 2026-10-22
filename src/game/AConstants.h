#pragma once

#include "common/abi.h"
#include "toshi/kernel/math/TVector3.h"

// The names of the game's tunable constants (SimJP.exe); the enumerators are named after their
// Data/Constant.ini keys (AConstantNames.inl).
class AConstantTypes
{
public:
	enum eFloatConstants
	{
#define JPOG_FLOAT_CONSTANT(name) name,
#include "game/AConstantNames.inl"
#undef JPOG_FLOAT_CONSTANT
		FLOAT_CONSTANT_COUNT
	};

	enum eIntConstants
	{
#define JPOG_INT_CONSTANT(name) name,
#include "game/AConstantNames.inl"
#undef JPOG_INT_CONSTANT
		INT_CONSTANT_COUNT
	};

	enum eVector3Constants
	{
#define JPOG_VECTOR3_CONSTANT(name) name,
#include "game/AConstantNames.inl"
#undef JPOG_VECTOR3_CONSTANT
		VECTOR3_CONSTANT_COUNT
	};

	static const char* sm_caszFloatConstantNames[FLOAT_CONSTANT_COUNT];
	static const char* sm_caszIntConstantNames[INT_CONSTANT_COUNT];
	static const char* sm_caszVector3ConstantNames[VECTOR3_CONSTANT_COUNT];
};

// The constants' values, read once from Data/Constant.ini (groups FloatConstants, IntConstants
// and Vector3Constants). Missing or mistyped keys are 0 (vectors TVector3::VEC_ZERO).
class AConstants
{
public:
	AConstants() = default;

	// Leaves every value unchanged (and IsInitialised false) when the file cannot be read.
	static void TSTDCALL Initialise();
	static bool IsInitialised() { return m_bInitialised; }

	static float TSTDCALL GetFloatConstant(AConstantTypes::eFloatConstants constant);
	static int TSTDCALL GetIntConstant(AConstantTypes::eIntConstants constant);
	static const Toshi::TVector3& TSTDCALL GetVector3Constant(AConstantTypes::eVector3Constants constant);

private:
	static float m_aFloatConstants[AConstantTypes::FLOAT_CONSTANT_COUNT];
	static int m_aIntConstants[AConstantTypes::INT_CONSTANT_COUNT];
	static Toshi::TVector3 m_aVector3Constants[AConstantTypes::VECTOR3_CONSTANT_COUNT];
	static bool m_bInitialised;
};
