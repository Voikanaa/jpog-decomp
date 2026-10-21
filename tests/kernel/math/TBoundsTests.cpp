#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/math/TAABox.h"
#include "toshi/kernel/math/TMatrix44.h"
#include "toshi/kernel/math/TSphere.h"

#include <cmath>
#include <cstring>
#include <doctest/doctest.h>

using Toshi::TAABox;
using Toshi::TMatrix44ID;
using Toshi::TPlane;
using Toshi::TSphere;
using Toshi::TVector3;
using Toshi::TVector4;
using namespace jpog::test;

namespace
{

// Bitwise equality of float-only objects.
template <class T> bool SameFloats(const T& a, const T& b)
{
	float x[sizeof(T) / 4], y[sizeof(T) / 4];
	std::memcpy(x, &a, sizeof(T));
	std::memcpy(y, &b, sizeof(T));
	for (size_t i = 0; i < sizeof(T) / 4; ++i)
		if (!Same(x[i], y[i]))
			return false;
	return true;
}

// Mostly ordinary values, sometimes zero or an exact repeat so ties and containment occur.
float Value(InputGenerator& gen, float range)
{
	switch (gen.NextU32() % 8)
	{
	case 0:
		return 0.0f;
	case 1:
		return static_cast<float>(gen.NextInt(-4, 4));
	default:
		return gen.NextFloat(-range, range);
	}
}

TSphere RandomSphere(InputGenerator& gen)
{
	return TSphere(Value(gen, 50.0f), Value(gen, 50.0f), Value(gen, 50.0f), std::abs(Value(gen, 40.0f)));
}

TAABox RandomBox(InputGenerator& gen)
{
	return TAABox(TVector3(Value(gen, 50.0f), Value(gen, 50.0f), Value(gen, 50.0f)),
	              TVector3(std::abs(Value(gen, 20.0f)), std::abs(Value(gen, 20.0f)), std::abs(Value(gen, 20.0f))));
}

TPlane RandomPlane(InputGenerator& gen)
{
	return TPlane(Value(gen, 1.0f), Value(gen, 1.0f), Value(gen, 1.0f), Value(gen, 60.0f));
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("Bounds")
{
	TEST_CASE("sphere")
	{
		TSphere a(0.0f, 0.0f, 0.0f, 1.0f);
		TSphere b(TVector3(4.0f, 0.0f, 0.0f), 1.0f);
		TSphere u;
		u.Union(a, b);
		CHECK(SameFloats(u.AsVector4(), TVector4(2.0f, 0.0f, 0.0f, 3.0f)));
		TSphere big(TVector4(0.0f, 0.0f, 0.0f, 10.0f));
		u.Union(a, big);
		CHECK(SameFloats(u, big));
		TPlane plane(1.0f, 0.0f, 0.0f, 0.0f);
		CHECK(TSphere(5.0f, 0.0f, 0.0f, 1.0f).ComparePlane(plane) == TPlane::PlaneComparison_Negative);
		CHECK(TSphere(-5.0f, 0.0f, 0.0f, 1.0f).ComparePlane(plane) == TPlane::PlaneComparison_Positive);
		CHECK(a.ComparePlane(plane) == TPlane::PlaneComparison_Intersects);
	}

	TEST_CASE("box and matrix ID")
	{
		TAABox boxes[2] = {TAABox(TVector3(0.0f, 0.0f, 0.0f), TVector3(1.0f, 1.0f, 1.0f)),
		                   TAABox(TVector3(4.0f, 0.0f, 0.0f), TVector3(1.0f, 2.0f, 1.0f))};
		TAABox box;
		box.BoxUnion(boxes, 2);
		CHECK(SameFloats(box.GetCentre(), TVector3(2.0f, 0.0f, 0.0f)));
		CHECK(SameFloats(box.GetBounds(), TVector3(3.0f, 2.0f, 1.0f)));
		TPlane plane(1.0f, 0.0f, 0.0f, 0.0f);
		CHECK(boxes[1].ComparePlane(plane) == TPlane::PlaneComparison_Positive);
		box.SetCentre(TVector3(-9.0f, 0.0f, 0.0f));
		CHECK(box.ComparePlane(plane) == TPlane::PlaneComparison_Negative);

		TMatrix44ID matrix;
		matrix.Identity();
		matrix.SetID(7);
		TMatrix44ID copy(matrix);
		CHECK(copy.GetID() == 7);
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

TEST_SUITE("Bounds differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("spheres and boxes match the original")
	{
		using P = void*;
		const auto unite = Original<Method<void, P, P>>("?Union@TSphere@Toshi@@QAEXAAV12@0@Z");
		const auto sphereCompare =
		    Original<Method<int, P>>("?ComparePlane@TSphere@Toshi@@QAE?AW4PlaneComparison@TPlane@2@AAV42@@Z");
		const auto boxCompare =
		    Original<Method<int, P>>("?ComparePlane@TAABox@Toshi@@QAE?AW4PlaneComparison@TPlane@2@AAV42@@Z");
		const auto boxUnion = Original<Method<void, P, unsigned int>>("?BoxUnion@TAABox@Toshi@@QAEXPAV12@I@Z");

		InputGenerator gen;
		GamePrecisionScope precision;
		for (int i = 0; i < 20000; ++i)
		{
			INFO("iteration ", i);
			// Union, also with `this` aliasing either input.
			TSphere spheres[2] = {RandomSphere(gen), RandomSphere(gen)};
			if (gen.NextU32() % 8 == 0)
				spheres[1] = spheres[0];
			const int alias = static_cast<int>(gen.NextU32() % 3);
			TSphere original[2] = {spheres[0], spheres[1]};
			TSphere ours[2] = {spheres[0], spheres[1]};
			TSphere originalResult(0.0f, 0.0f, 0.0f, 0.0f), oursResult(0.0f, 0.0f, 0.0f, 0.0f);
			unite(alias == 2 ? static_cast<P>(&originalResult) : static_cast<P>(&original[alias]), &original[0],
			      &original[1]);
			(alias == 2 ? oursResult : ours[alias]).Union(ours[0], ours[1]);
			if (alias == 2)
				CHECK(SameFloats(originalResult, oursResult));
			CHECK(SameFloats(original[0], ours[0]));
			CHECK(SameFloats(original[1], ours[1]));

			TPlane plane = RandomPlane(gen);
			CHECK(sphereCompare(&spheres[0], &plane) == spheres[0].ComparePlane(plane));
			TAABox box = RandomBox(gen);
			CHECK(boxCompare(&box, &plane) == box.ComparePlane(plane));

			TAABox boxes[6];
			const unsigned int count = static_cast<unsigned int>(gen.NextInt(0, 6));
			for (TAABox& entry : boxes)
				entry = gen.NextU32() % 6 == 0 ? boxes[0] : RandomBox(gen);
			TAABox originalBox = box, oursBox = box;
			TAABox* input = gen.NextU32() % 16 == 0 ? nullptr : boxes;
			boxUnion(&originalBox, input, count);
			oursBox.BoxUnion(input, count);
			CHECK(SameFloats(originalBox, oursBox));
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
