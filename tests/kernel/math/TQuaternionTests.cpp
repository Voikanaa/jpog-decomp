#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/math/TMath.h"
#include "toshi/kernel/math/TQuaternion.h"
#include "toshi/kernel/math/TVector3.h"
#include "toshi/kernel/math/TVector4.h"

#include <doctest/doctest.h>

using Toshi::TMath;
using Toshi::TQuaternion;
using Toshi::TVector3;
using Toshi::TVector4;
using namespace jpog::test;

namespace
{

using jpog::test::Same;

bool Same(const TQuaternion& a, const TQuaternion& b)
{
	return Same(a.x, b.x) && Same(a.y, b.y) && Same(a.z, b.z) && Same(a.w, b.w);
}

bool Same(const TVector3& a, const TVector3& b)
{
	return Same(a.x, b.x) && Same(a.y, b.y) && Same(a.z, b.z);
}

bool Same(const TVector4& a, const TVector4& b)
{
	return Same(a.x, b.x) && Same(a.y, b.y) && Same(a.z, b.z) && Same(a.w, b.w);
}

doctest::String ToString(const TQuaternion& q)
{
	return doctest::toString(q.x) + ", " + doctest::toString(q.y) + ", " + doctest::toString(q.z) + ", " +
	       doctest::toString(q.w);
}

TVector3 RandomVec3(InputGenerator& gen, float range = 10.0f)
{
	return TVector3(gen.NextFloat(-range, range), gen.NextFloat(-range, range), gen.NextFloat(-range, range));
}

TVector3 RandomAxis(InputGenerator& gen)
{
	TVector3 axis = RandomVec3(gen);
	axis.Normalise();
	return axis;
}

// Mostly unit quaternions (the normal use), sometimes arbitrary ones.
TQuaternion RandomQuat(InputGenerator& gen)
{
	TQuaternion q(gen.NextFloat(-2.0f, 2.0f), gen.NextFloat(-2.0f, 2.0f), gen.NextFloat(-2.0f, 2.0f),
	              gen.NextFloat(-2.0f, 2.0f));
	if (gen.NextU32() % 4 != 0)
		q.Normalise();
	return q;
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TQuaternion")
{
	TEST_CASE("rotation basics")
	{
		// A quarter turn around Z maps +X to +Y.
		const TQuaternion quarterZ(TVector3::VEC_POSZ, TMath::HALF_PI);
		TVector3 rotated;
		TQuaternion::RotateVector(rotated, quarterZ, TVector3::VEC_POSX);
		CHECK(rotated.x == doctest::Approx(0.0f).epsilon(1e-6));
		CHECK(rotated.y == doctest::Approx(1.0f));
		CHECK(rotated.z == doctest::Approx(0.0f).epsilon(1e-6));

		TQuaternion q = TQuaternion::IDENTITY;
		q *= quarterZ;
		CHECK(q == quarterZ);
		CHECK(TQuaternion::IDENTITY.Magnitude() == 1.0f);

		TQuaternion negative(0.0f, 0.0f, 0.5f, -0.5f);
		negative.MakePrincple();
		CHECK(negative == TQuaternion(-0.0f, -0.0f, -0.5f, 0.5f));
	}

	TEST_CASE("SetVectorDirection")
	{
		TQuaternion q;
		q.SetVectorDirection(TVector3::VEC_POSY, TVector3::VEC_POSX);
		TVector3 rotated;
		TQuaternion::RotateVector(rotated, q, TVector3::VEC_POSX);
		CHECK(rotated.y == doctest::Approx(1.0f));
		q.SetVectorDirection(TVector3::VEC_POSX, TVector3::VEC_POSX);
		CHECK(q == TQuaternion::IDENTITY);
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

namespace
{

constexpr int kIterations = 2000;

using Unary = Method<void>;
using WithQuat = Method<void, const TQuaternion*>;
using WithTwoQuatsFloat = Method<void, const TQuaternion*, const TQuaternion*, float>;
using WithQuatFloat = Method<void, const TQuaternion*, float>;
using WithAxisAngle = Method<void, const TVector3*, float>;
using RefWithQuat = Method<TQuaternion*, const TQuaternion*>;
using RefWithFloat = Method<TQuaternion*, float>;
using ValueWithQuat = Method<TQuaternion*, TQuaternion*, const TQuaternion*>;
using ValueWithFloat = Method<TQuaternion*, TQuaternion*, float>;
using StaticTernary = void(__stdcall*)(TQuaternion*, const TQuaternion*, const TQuaternion*);

// Runs `original` and `ours` on identical copies of a random quaternion and checks that the
// results match bit for bit.
template <typename Call> void CheckMutator(const char* name, Call call)
{
	InputGenerator gen;
	for (int i = 0; i < kIterations; ++i)
	{
		TQuaternion original = RandomQuat(gen);
		TQuaternion ours = original;
		const TQuaternion before = original;
		call(gen, original, ours);
		INFO(doctest::String(name), ": self=(", ToString(before), ") original=(", ToString(original), ") ours=(",
		     ToString(ours), ")");
		CHECK(Same(original, ours));
	}
}

} // namespace

TEST_SUITE("TQuaternion differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("construction and setters")
	{
		GamePrecisionScope precision;
		const auto setIdentity = Original<Unary>("?SetIdentity@TQuaternion@Toshi@@QAEXXZ");
		const auto set = Original<Method<void, float, float, float, float>>("?Set@TQuaternion@Toshi@@QAEXMMMM@Z");
		const auto setRotation = Original<WithAxisAngle>("?SetRotation@TQuaternion@Toshi@@QAEXABVTVector3@2@M@Z");
		const auto construct = Original<WithAxisAngle>("??0TQuaternion@Toshi@@QAE@ABVTVector3@1@M@Z");
		const auto eulerYX = Original<Method<void, const float*>>("?SetFromEulerYX@TQuaternion@Toshi@@QAEXPBM@Z");
		const auto rollPitchYaw =
		    Original<Method<void, float, float, float>>("?SetFromEulerRollPitchYaw@TQuaternion@Toshi@@QAEXMMM@Z");
		const char* const kinds[] = {"SetIdentity",    "Set",
		                             "SetRotation",    "TQuaternion(axis, angle)",
		                             "SetFromEulerYX", "SetFromEulerRollPitchYaw"};
		for (unsigned int kind = 0; kind < 6; ++kind)
		{
			CheckMutator(kinds[kind],
			             [&](InputGenerator& g, TQuaternion& o, TQuaternion& m)
			             {
				             const TVector3 axis = RandomAxis(g);
				             const float a = g.NextFloat(-7.0f, 7.0f), b = g.NextFloat(-7.0f, 7.0f),
				                         c = g.NextFloat(-7.0f, 7.0f);
				             const float angles[2] = {a, b};
				             switch (kind)
				             {
				             case 0:
					             setIdentity(&o);
					             m.SetIdentity();
					             break;
				             case 1:
					             set(&o, a, b, c, -a);
					             m.Set(a, b, c, -a);
					             break;
				             case 2:
					             setRotation(&o, &axis, a);
					             m.SetRotation(axis, a);
					             break;
				             case 3:
					             construct(&o, &axis, a);
					             m = TQuaternion(axis, a);
					             break;
				             case 4:
					             eulerYX(&o, angles);
					             m.SetFromEulerYX(angles);
					             break;
				             default:
					             rollPitchYaw(&o, a, b, c);
					             m.SetFromEulerRollPitchYaw(a, b, c);
					             break;
				             }
			             });
		}
	}

	TEST_CASE("SetVectorDirection")
	{
		GamePrecisionScope precision;
		const auto fn = Original<Method<void, const TVector3*, const TVector3*>>(
		    "?SetVectorDirection@TQuaternion@Toshi@@QAEXABVTVector3@2@0@Z");
		CheckMutator("SetVectorDirection",
		             [&](InputGenerator& g, TQuaternion& o, TQuaternion& m)
		             {
			             const TVector3 to = RandomAxis(g);
			             TVector3 from = RandomAxis(g);
			             // Exercise the parallel and opposite branches, including nearly vertical vectors.
			             switch (g.NextU32() % 6)
			             {
			             case 0:
				             from = to;
				             break;
			             case 1:
				             from.Negate(to);
				             break;
			             case 2:
				             from = TVector3::VEC_POSY;
				             break;
			             default:
				             break;
			             }
			             const TVector3 target = (g.NextU32() % 8 == 0) ? TVector3::VEC_NEGY : to;
			             fn(&o, &target, &from);
			             m.SetVectorDirection(target, from);
		             });
	}

	TEST_CASE("in-place operations")
	{
		GamePrecisionScope precision;
		const struct
		{
			const char* mangled;
			void (TQuaternion::*ours)();
		} unary[] = {
		    {"?SetConjugate@TQuaternion@Toshi@@QAEXXZ", &TQuaternion::SetConjugate},
		    {"?SetInverse@TQuaternion@Toshi@@QAEXXZ", &TQuaternion::SetInverse},
		    {"?Negate@TQuaternion@Toshi@@QAEXXZ", &TQuaternion::Negate},
		    {"?MakePrincple@TQuaternion@Toshi@@QAEXXZ", &TQuaternion::MakePrincple},
		    {"?Normalise@TQuaternion@Toshi@@QAEXXZ", &TQuaternion::Normalise},
		};
		for (const auto& c : unary)
		{
			const auto fn = Original<Unary>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TQuaternion& o, TQuaternion& m)
			             {
				             // Zero components test MakePrincple's scan and Normalise's zero guard.
				             for (float* component : {&o.x, &o.y, &o.z, &o.w})
				             {
					             if (g.NextU32() % 3 == 0)
						             *component = (g.NextU32() & 1) ? 0.0f : -0.0f;
				             }
				             m = o;
				             fn(&o);
				             (m.*c.ours)();
			             });
		}

		const auto rotateAroundAxis =
		    Original<WithAxisAngle>("?RotateAroundAxis@TQuaternion@Toshi@@QAEXABVTVector3@2@M@Z");
		CheckMutator("RotateAroundAxis",
		             [&](InputGenerator& g, TQuaternion& o, TQuaternion& m)
		             {
			             const TVector3 axis = RandomAxis(g);
			             const float angle = g.NextFloat(-7.0f, 7.0f);
			             rotateAroundAxis(&o, &axis, angle);
			             m.RotateAroundAxis(axis, angle);
		             });
	}

	TEST_CASE("interpolation")
	{
		GamePrecisionScope precision;
		const auto slerp = Original<WithTwoQuatsFloat>("?Slerp@TQuaternion@Toshi@@QAEXABV12@0M@Z");
		const auto slerpTo = Original<WithQuatFloat>("?Slerp@TQuaternion@Toshi@@QAEXABV12@M@Z");
		const auto lerp = Original<WithTwoQuatsFloat>("?Lerp@TQuaternion@Toshi@@QAEXABV12@0M@Z");
		const char* const kinds[] = {"Slerp(from, to, t)", "Slerp(to, t)", "Lerp"};
		for (unsigned int kind = 0; kind < 3; ++kind)
		{
			CheckMutator(kinds[kind],
			             [&](InputGenerator& g, TQuaternion& o, TQuaternion& m)
			             {
				             const TQuaternion a = RandomQuat(g);
				             TQuaternion b = RandomQuat(g);
				             // Nearly identical (or opposite) quaternions take the linear fallback.
				             if (g.NextU32() % 4 == 0)
				             {
					             b = a;
					             b.x += g.NextFloat(-0.01f, 0.01f);
					             if (g.NextU32() & 1)
						             b.Negate();
				             }
				             const float t = g.NextFloat(0.0f, 1.0f);
				             switch (kind)
				             {
				             case 0:
					             slerp(&o, &a, &b, t);
					             m.Slerp(a, b, t);
					             break;
				             case 1:
					             slerpTo(&o, &b, t);
					             m.Slerp(b, t);
					             break;
				             default:
					             lerp(&o, &a, &b, t);
					             m.Lerp(a, b, t);
					             break;
				             }
			             });
		}
	}

	TEST_CASE("operators")
	{
		GamePrecisionScope precision;
		const struct
		{
			const char* mangled;
			TQuaternion& (TQuaternion::*ours)(const TQuaternion&);
		} compoundQuat[] = {{"??YTQuaternion@Toshi@@QAEAAV01@ABV01@@Z", &TQuaternion::operator+= },
		                     {"??ZTQuaternion@Toshi@@QAEAAV01@ABV01@@Z", &TQuaternion::operator-= },
		                      {"??XTQuaternion@Toshi@@QAEAAV01@ABV01@@Z", &TQuaternion::operator*= },
		                       { "??_0TQuaternion@Toshi@@QAEAAV01@ABV01@@Z",
			                     &TQuaternion::operator/= },
		                      };
		for (const auto& c : compoundQuat)
		{
			const auto fn = Original<RefWithQuat>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TQuaternion& o, TQuaternion& m)
			             {
				             const TQuaternion q = RandomQuat(g);
				             CHECK(fn(&o, &q) == &o);
				             (m.*c.ours)(q);
			             });
		}
		const struct
		{
			const char* mangled;
			TQuaternion& (TQuaternion::*ours)(float);
		} compoundFloat[] = {{"??XTQuaternion@Toshi@@QAEAAV01@M@Z", &TQuaternion::operator*= },
		                      { "??_0TQuaternion@Toshi@@QAEAAV01@M@Z",
			                    &TQuaternion::operator/= },
		};
		for (const auto& c : compoundFloat)
		{
			const auto fn = Original<RefWithFloat>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TQuaternion& o, TQuaternion& m)
			             {
				             const float s = (g.NextU32() % 8 == 0) ? 0.0f : g.NextFloat(-10.0f, 10.0f);
				             CHECK(fn(&o, s) == &o);
				             (m.*c.ours)(s);
			             });
		}
		const struct
		{
			const char* mangled;
			TQuaternion (TQuaternion::*ours)(const TQuaternion&) const;
		} valueQuat[] = {{"??HTQuaternion@Toshi@@QBE?AV01@ABV01@@Z", &TQuaternion::operator+ },
		                  {"??GTQuaternion@Toshi@@QBE?AV01@ABV01@@Z", &TQuaternion::operator- },
		                   {"??DTQuaternion@Toshi@@QBE?AV01@ABV01@@Z", &TQuaternion::operator* },
		                    { "??KTQuaternion@Toshi@@QBE?AV01@ABV01@@Z",
			                  &TQuaternion::operator/ },
		                   };
		for (const auto& c : valueQuat)
		{
			const auto fn = Original<ValueWithQuat>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TQuaternion& o, TQuaternion& m)
			             {
				             const TQuaternion lhs = RandomQuat(g), rhs = RandomQuat(g);
				             TQuaternion result;
				             CHECK(fn(&lhs, &result, &rhs) == &result);
				             o = result;
				             m = (lhs.*c.ours)(rhs);
			             });
		}
		const struct
		{
			const char* mangled;
			TQuaternion (TQuaternion::*ours)(float) const;
		} valueFloat[] = {{"??DTQuaternion@Toshi@@QBE?AV01@M@Z", &TQuaternion::operator* },
		                   { "??KTQuaternion@Toshi@@QBE?AV01@M@Z",
			                 &TQuaternion::operator/ },
		};
		for (const auto& c : valueFloat)
		{
			const auto fn = Original<ValueWithFloat>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TQuaternion& o, TQuaternion& m)
			             {
				             const TQuaternion lhs = RandomQuat(g);
				             const float s = (g.NextU32() % 8 == 0) ? 0.0f : g.NextFloat(-10.0f, 10.0f);
				             TQuaternion result;
				             CHECK(fn(&lhs, &result, s) == &result);
				             o = result;
				             m = (lhs.*c.ours)(s);
			             });
		}
		const struct
		{
			const char* mangled;
			void(TSTDCALL* ours)(TQuaternion&, const TQuaternion&, const TQuaternion&);
		} statics[] = {
		    {"?Add@TQuaternion@Toshi@@SGXAAV12@ABV12@1@Z", &TQuaternion::Add},
		    {"?Subtract@TQuaternion@Toshi@@SGXAAV12@ABV12@1@Z", &TQuaternion::Subtract},
		    {"?Multiply@TQuaternion@Toshi@@SGXAAV12@ABV12@1@Z", &TQuaternion::Multiply},
		    {"?Divide@TQuaternion@Toshi@@SGXAAV12@ABV12@1@Z", &TQuaternion::Divide},
		};
		for (const auto& c : statics)
		{
			const auto fn = Original<StaticTernary>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TQuaternion& o, TQuaternion& m)
			             {
				             const TQuaternion a = RandomQuat(g), b = RandomQuat(g);
				             fn(&o, &a, &b);
				             c.ours(m, a, b);
			             });
		}
	}

	TEST_CASE("queries and vector rotation")
	{
		GamePrecisionScope precision;
		const auto magnitude = Original<Method<float>>("?Magnitude@TQuaternion@Toshi@@QBEMXZ");
		const auto magnitudeSq = Original<Method<float>>("?MagnitudeSq@TQuaternion@Toshi@@QBEMXZ");
		const auto equal = Original<Method<bool, const TQuaternion*>>("??8TQuaternion@Toshi@@QBE_NABV01@@Z");
		const auto notEqual = Original<Method<bool, const TQuaternion*>>("??9TQuaternion@Toshi@@QBE_NABV01@@Z");
		const auto direction =
		    Original<Method<void, TVector3*>>("?GetVectorDirection@TQuaternion@Toshi@@QBEXAAVTVector3@2@@Z");
		using Rotate3 = void(__stdcall*)(TVector3*, const TQuaternion*, const TVector3*);
		using Rotate4 = void(__stdcall*)(TVector4*, const TQuaternion*, const TVector4*);
		const auto rotate3 = Original<Rotate3>("?RotateVector@TQuaternion@Toshi@@SGXAAVTVector3@2@ABV12@ABV32@@Z");
		const auto rotate4 = Original<Rotate4>("?RotateVector@TQuaternion@Toshi@@SGXAAVTVector4@2@ABV12@ABV32@@Z");
		InputGenerator gen;
		for (int i = 0; i < kIterations; ++i)
		{
			const TQuaternion a = RandomQuat(gen);
			const TQuaternion b = (i % 4 == 0) ? a : RandomQuat(gen);
			const TVector3 v3 = RandomVec3(gen);
			const TVector4 v4(v3.x, v3.y, v3.z, gen.NextFloat(-2.0f, 2.0f));
			INFO("a=(", ToString(a), ") b=(", ToString(b), ")");
			CHECK(Same(magnitude(&a), a.Magnitude()));
			CHECK(Same(magnitudeSq(&a), a.MagnitudeSq()));
			CHECK(equal(&a, &b) == (a == b));
			CHECK(notEqual(&a, &b) == (a != b));

			TVector3 original3, ours3;
			direction(&a, &original3);
			a.GetVectorDirection(ours3);
			CHECK(Same(original3, ours3));
			rotate3(&original3, &a, &v3);
			TQuaternion::RotateVector(ours3, a, v3);
			CHECK(Same(original3, ours3));
			TVector4 original4(0.0f, 0.0f, 0.0f, 0.0f), ours4(0.0f, 0.0f, 0.0f, 0.0f);
			rotate4(&original4, &a, &v4);
			TQuaternion::RotateVector(ours4, a, v4);
			CHECK(Same(original4, ours4));
		}

		const TQuaternion q(1.0f, 2.0f, 3.0f, 4.0f);
		const auto asVector4 = Original<Method<const TVector4*>>("?AsVector4@TQuaternion@Toshi@@QBEABVTVector4@2@XZ");
		CHECK(asVector4(&q) == &q.AsVector4());
	}

	TEST_CASE("static constants")
	{
		const auto* identity =
		    static_cast<const TQuaternion*>(OriginalModule::Kernel().Find("?IDENTITY@TQuaternion@Toshi@@2V12@B"));
		const auto* zeroRotation = static_cast<const TVector3*>(
		    OriginalModule::Kernel().Find("?VEC3_ZERO_ROTATION@TQuaternion@Toshi@@2VTVector3@2@B"));
		const auto* delta = static_cast<const float*>(OriginalModule::Kernel().Find("?DELTA@TQuaternion@Toshi@@2MB"));
		REQUIRE(identity != nullptr);
		REQUIRE(zeroRotation != nullptr);
		REQUIRE(delta != nullptr);
		INFO("IDENTITY=(", ToString(*identity), ") VEC3_ZERO_ROTATION=(", zeroRotation->x, ", ", zeroRotation->y, ", ",
		     zeroRotation->z, ")");
		CHECK(Same(*identity, TQuaternion::IDENTITY));
		CHECK(Same(*zeroRotation, TQuaternion::VEC3_ZERO_ROTATION));
		CHECK(Same(*delta, TQuaternion::DELTA));
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
