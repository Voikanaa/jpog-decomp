#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TVector2.h"
#include "toshi/kernel/TVector3.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <doctest/doctest.h>
#include <initializer_list>
#include <utility>

using Toshi::TVector2;
using Toshi::TVector3;
using namespace jpog::test;

namespace
{

// Bitwise equality (also distinguishes +0/-0); any two NaNs compare equal.
bool Same(float a, float b)
{
	return std::bit_cast<uint32_t>(a) == std::bit_cast<uint32_t>(b) || (std::isnan(a) && std::isnan(b));
}

bool Same(const TVector2& a, const TVector2& b)
{
	return Same(a.x, b.x) && Same(a.y, b.y);
}

bool Same(const TVector3& a, const TVector3& b)
{
	return Same(a.x, b.x) && Same(a.y, b.y) && Same(a.z, b.z);
}

doctest::String ToString(const TVector2& v)
{
	return doctest::toString(v.x) + ", " + doctest::toString(v.y);
}

doctest::String ToString(const TVector3& v)
{
	return doctest::toString(v.x) + ", " + doctest::toString(v.y) + ", " + doctest::toString(v.z);
}

TVector2 Random2(InputGenerator& gen, float range = 100.0f)
{
	return TVector2(gen.NextFloat(-range, range), gen.NextFloat(-range, range));
}

TVector3 Random3(InputGenerator& gen, float range = 100.0f)
{
	return TVector3(gen.NextFloat(-range, range), gen.NextFloat(-range, range), gen.NextFloat(-range, range));
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TVector")
{
	TEST_CASE("TVector2 basics")
	{
		TVector2 v(3.0f, 4.0f);
		CHECK(v.Magnitude() == 5.0f);
		v.Normalise();
		CHECK(v.IsNormalised(0.001f));
		TVector2 zero(0.0f, 0.0f);
		zero.Normalise();
		CHECK(zero == TVector2::VEC_POSX);
		CHECK(TVector2::CrossProduct(TVector2::VEC_POSX, TVector2::VEC_POSY) == 1.0f);
		CHECK((TVector2(1.0f, 2.0f) + TVector2(3.0f, 4.0f)) == TVector2(4.0f, 6.0f));
		CHECK(TVector2(1.0f, 2.0f).GetVector3XZ() == TVector3(1.0f, 0.0f, 2.0f));
	}

	TEST_CASE("TVector3 basics")
	{
		TVector3 cross;
		cross.CrossProduct(TVector3::VEC_POSX, TVector3::VEC_POSY);
		CHECK(cross == TVector3::VEC_POSZ);
		TVector3 zero(0.0f, 0.0f, 0.0f);
		zero.Normalise();
		CHECK(zero == TVector3::VEC_POSZ);
		CHECK(TVector3(1.0f, 2.0f, 2.0f).Magnitude() == 3.0f);
		CHECK(TVector3(1.0f, -2.0f, 3.0f).IsReasonable(10.0f));
		CHECK_FALSE(TVector3(10.0f, 0.0f, 0.0f).IsReasonable(10.0f));
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

namespace
{

bool KernelMissing()
{
	return !OriginalModule::Kernel().IsLoaded();
}

template <typename Fn> Fn Original(const char* mangledName)
{
	const Fn fn = OriginalModule::Kernel().Get<Fn>(mangledName);
	REQUIRE_MESSAGE(fn != nullptr, "export not found: " << mangledName);
	return fn;
}

constexpr int kIterations = 2000;

// Function shapes of the original exports. Members are __thiscall (called through Method<>); functions returning
// a vector by value take a hidden result pointer as their first stack argument.
template <typename V> using Unary = Method<void>;
template <typename V> using WithVec = Method<void, const V*>;
template <typename V> using WithTwoVecs = Method<void, const V*, const V*>;
template <typename V> using WithFloat = Method<void, float>;
template <typename V> using WithVecFloat = Method<void, const V*, float>;
template <typename V> using WithTwoVecsFloat = Method<void, const V*, const V*, float>;
template <typename V> using RefWithVec = Method<V*, const V*>;
template <typename V> using RefWithFloat = Method<V*, float>;
template <typename V> using ValueWithVec = Method<V*, V*, const V*>;
template <typename V> using ValueWithFloat = Method<V*, V*, float>;
template <typename V> using QueryFloat = Method<float>;
template <typename V> using QueryFloatVec = Method<float, const V*>;
template <typename V> using QueryBoolVec = Method<bool, const V*>;
template <typename V> using QueryBoolFloat = Method<bool, float>;
template <typename V> using StaticFloatVecVec = float(__stdcall*)(const V*, const V*);

// Runs `original` and `ours` on identical copies of a random `self` (plus arguments produced by
// `call`) and checks that the resulting vectors match bit for bit.
template <typename V, typename Rand, typename Call> void CheckMutator(const char* name, Rand random, Call call)
{
	InputGenerator gen;
	for (int i = 0; i < kIterations; ++i)
	{
		V original = random(gen);
		V ours = original;
		const V before = original;
		call(gen, original, ours);
		INFO(name, ": self=(", ToString(before), ") original=(", ToString(original), ") ours=(", ToString(ours), ")");
		CHECK(Same(original, ours));
	}
}

template <typename V> V MakeRandom(InputGenerator& gen);
template <> TVector2 MakeRandom<TVector2>(InputGenerator& gen)
{
	return Random2(gen);
}
template <> TVector3 MakeRandom<TVector3>(InputGenerator& gen)
{
	return Random3(gen);
}

// Shared checks for the members TVector2 and TVector3 have in common. `names` holds the
// class-specific part of each mangled name.
template <typename V> void CheckCommonMembers(const char* cls)
{
	auto name = [&](const char* fmt)
	{
		static char buffer[160];
		std::snprintf(buffer, sizeof(buffer), fmt, cls);
		return buffer;
	};
	auto rand = [](InputGenerator& g) { return MakeRandom<V>(g); };

	{
		const auto fn = Original<WithVec<V>>(name("?Add@%s@Toshi@@QAEXABV12@@Z"));
		CheckMutator<V>("Add(v)", rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const V v = MakeRandom<V>(g);
			                fn(&o, &v);
			                m.Add(v);
		                });
	}
	{
		const auto fn = Original<WithVec<V>>(name("?Subtract@%s@Toshi@@QAEXABV12@@Z"));
		CheckMutator<V>("Subtract(v)", rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const V v = MakeRandom<V>(g);
			                fn(&o, &v);
			                m.Subtract(v);
		                });
	}
	{
		const auto fn = Original<WithVec<V>>(name("?Multiply@%s@Toshi@@QAEXABV12@@Z"));
		CheckMutator<V>("Multiply(v)", rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const V v = MakeRandom<V>(g);
			                fn(&o, &v);
			                m.Multiply(v);
		                });
	}
	{
		const auto fn = Original<WithVec<V>>(name("?Divide@%s@Toshi@@QAEXABV12@@Z"));
		CheckMutator<V>("Divide(v)", rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const V v = MakeRandom<V>(g);
			                fn(&o, &v);
			                m.Divide(v);
		                });
	}
	{
		const auto fn = Original<WithTwoVecs<V>>(name("?Add@%s@Toshi@@QAEXABV12@0@Z"));
		CheckMutator<V>("Add(a,b)", rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const V a = MakeRandom<V>(g), b = MakeRandom<V>(g);
			                fn(&o, &a, &b);
			                m.Add(a, b);
		                });
	}
	{
		const auto fn = Original<WithTwoVecs<V>>(name("?Subtract@%s@Toshi@@QAEXABV12@0@Z"));
		CheckMutator<V>("Subtract(a,b)", rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const V a = MakeRandom<V>(g), b = MakeRandom<V>(g);
			                fn(&o, &a, &b);
			                m.Subtract(a, b);
		                });
	}
	{
		const auto fn = Original<WithTwoVecs<V>>(name("?Multiply@%s@Toshi@@QAEXABV12@0@Z"));
		CheckMutator<V>("Multiply(a,b)", rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const V a = MakeRandom<V>(g), b = MakeRandom<V>(g);
			                fn(&o, &a, &b);
			                m.Multiply(a, b);
		                });
	}
	{
		const auto fn = Original<WithTwoVecs<V>>(name("?Divide@%s@Toshi@@QAEXABV12@0@Z"));
		CheckMutator<V>("Divide(a,b)", rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const V a = MakeRandom<V>(g), b = MakeRandom<V>(g);
			                fn(&o, &a, &b);
			                m.Divide(a, b);
		                });
	}
	{
		const auto fn = Original<WithFloat<V>>(name("?Multiply@%s@Toshi@@QAEXM@Z"));
		CheckMutator<V>("Multiply(s)", rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const float s = g.NextFloat(-10.0f, 10.0f);
			                fn(&o, s);
			                m.Multiply(s);
		                });
	}
	{
		const auto fn = Original<WithFloat<V>>(name("?Divide@%s@Toshi@@QAEXM@Z"));
		CheckMutator<V>("Divide(s)", rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const float s = g.NextFloat(-10.0f, 10.0f);
			                fn(&o, s);
			                m.Divide(s);
		                });
	}
	{
		const auto fn = Original<WithVecFloat<V>>(name("?Multiply@%s@Toshi@@QAEXABV12@M@Z"));
		CheckMutator<V>("Multiply(v,s)", rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const V v = MakeRandom<V>(g);
			                const float s = g.NextFloat(-10.0f, 10.0f);
			                fn(&o, &v, s);
			                m.Multiply(v, s);
		                });
	}
	{
		const auto fn = Original<WithVecFloat<V>>(name("?Divide@%s@Toshi@@QAEXABV12@M@Z"));
		CheckMutator<V>("Divide(v,s)", rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const V v = MakeRandom<V>(g);
			                const float s = g.NextFloat(-10.0f, 10.0f);
			                fn(&o, &v, s);
			                m.Divide(v, s);
		                });
	}
	{
		const auto fn = Original<WithTwoVecsFloat<V>>(name("?Lerp@%s@Toshi@@QAEXABV12@0M@Z"));
		CheckMutator<V>("Lerp(a,b,t)", rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const V a = MakeRandom<V>(g), b = MakeRandom<V>(g);
			                const float t = g.NextFloat(0.0f, 1.0f);
			                fn(&o, &a, &b, t);
			                m.Lerp(a, b, t);
		                });
	}
	{
		const auto fn = Original<Unary<V>>(name("?Negate@%s@Toshi@@QAEXXZ"));
		CheckMutator<V>("Negate()", rand,
		                [&](InputGenerator&, V& o, V& m)
		                {
			                fn(&o);
			                m.Negate();
		                });
	}
	{
		const auto fn = Original<Unary<V>>(name("?Abs@%s@Toshi@@QAEXXZ"));
		CheckMutator<V>("Abs()", rand,
		                [&](InputGenerator&, V& o, V& m)
		                {
			                fn(&o);
			                m.Abs();
		                });
	}
	{
		const auto fn = Original<Unary<V>>(name("?Normalise@%s@Toshi@@QAEXXZ"));
		CheckMutator<V>("Normalise()", rand,
		                [&](InputGenerator&, V& o, V& m)
		                {
			                fn(&o);
			                m.Normalise();
		                });
	}
	{
		const auto fn = Original<WithVec<V>>(name("?Negate@%s@Toshi@@QAEXABV12@@Z"));
		CheckMutator<V>("Negate(v)", rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const V v = MakeRandom<V>(g);
			                fn(&o, &v);
			                m.Negate(v);
		                });
	}
	{
		const auto fn = Original<WithVec<V>>(name("?Abs@%s@Toshi@@QAEXABV12@@Z"));
		CheckMutator<V>("Abs(v)", rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const V v = MakeRandom<V>(g);
			                fn(&o, &v);
			                m.Abs(v);
		                });
	}

	// Compound assignment and by-value operators.
	const struct
	{
		const char* fmt;
		V& (V::*ours)(const V&);
	} compoundVec[] = {{"??Y%s@Toshi@@QAEAAV01@ABV01@@Z", &V::operator+= },
	                    {"??Z%s@Toshi@@QAEAAV01@ABV01@@Z", &V::operator-= },
	                     {"??X%s@Toshi@@QAEAAV01@ABV01@@Z", &V::operator*= },
	                      { "??_0%s@Toshi@@QAEAAV01@ABV01@@Z",
		                    &V::operator/= },
	                     };
	for (const auto& c : compoundVec)
	{
		const auto fn = Original<RefWithVec<V>>(name(c.fmt));
		CheckMutator<V>(c.fmt, rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const V v = MakeRandom<V>(g);
			                CHECK(fn(&o, &v) == &o);
			                (m.*c.ours)(v);
		                });
	}
	const struct
	{
		const char* fmt;
		V& (V::*ours)(float);
	} compoundFloat[] = {{"??X%s@Toshi@@QAEAAV01@M@Z", &V::operator*= },
	                      { "??_0%s@Toshi@@QAEAAV01@M@Z",
		                    &V::operator/= },
	};
	for (const auto& c : compoundFloat)
	{
		const auto fn = Original<RefWithFloat<V>>(name(c.fmt));
		CheckMutator<V>(c.fmt, rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const float s = g.NextFloat(-10.0f, 10.0f);
			                CHECK(fn(&o, s) == &o);
			                (m.*c.ours)(s);
		                });
	}
	const struct
	{
		const char* fmt;
		V (V::*ours)(const V&) const;
	} valueVec[] = {{"??H%s@Toshi@@QBE?AV01@ABV01@@Z", &V::operator+ },
	                 {"??G%s@Toshi@@QBE?AV01@ABV01@@Z", &V::operator- },
	                  {"??D%s@Toshi@@QBE?AV01@ABV01@@Z", &V::operator* },
	                   { "??K%s@Toshi@@QBE?AV01@ABV01@@Z",
		                 &V::operator/ },
	                  };
	for (const auto& c : valueVec)
	{
		const auto fn = Original<ValueWithVec<V>>(name(c.fmt));
		CheckMutator<V>(c.fmt, rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const V lhs = MakeRandom<V>(g), rhs = MakeRandom<V>(g);
			                V result;
			                CHECK(fn(&lhs, &result, &rhs) == &result);
			                o = result;
			                m = (lhs.*c.ours)(rhs);
		                });
	}
	const struct
	{
		const char* fmt;
		V (V::*ours)(float) const;
	} valueFloat[] = {{"??D%s@Toshi@@QBE?AV01@M@Z", &V::operator* },
	                   { "??K%s@Toshi@@QBE?AV01@M@Z",
		                 &V::operator/ },
	};
	for (const auto& c : valueFloat)
	{
		const auto fn = Original<ValueWithFloat<V>>(name(c.fmt));
		CheckMutator<V>(c.fmt, rand,
		                [&](InputGenerator& g, V& o, V& m)
		                {
			                const V lhs = MakeRandom<V>(g);
			                const float s = g.NextFloat(-10.0f, 10.0f);
			                V result;
			                CHECK(fn(&lhs, &result, s) == &result);
			                o = result;
			                m = (lhs.*c.ours)(s);
		                });
	}

	// Queries.
	const auto magnitude = Original<QueryFloat<V>>(name("?Magnitude@%s@Toshi@@QBEMXZ"));
	const auto magnitudeSq = Original<QueryFloat<V>>(name("?MagnitudeSq@%s@Toshi@@QBEMXZ"));
	const auto dotMember = Original<QueryFloatVec<V>>(name("?DotProduct@%s@Toshi@@QBEMABV12@@Z"));
	const auto dotStatic = Original<StaticFloatVecVec<V>>(name("?DotProduct@%s@Toshi@@SGMABV12@0@Z"));
	const auto distance = Original<StaticFloatVecVec<V>>(name("?Distance@%s@Toshi@@SGMABV12@0@Z"));
	const auto distanceSq = Original<StaticFloatVecVec<V>>(name("?DistanceSq@%s@Toshi@@SGMABV12@0@Z"));
	const auto isEqual = Original<QueryBoolVec<V>>(name("?IsEqual@%s@Toshi@@QBE_NABV12@@Z"));
	const auto equal = Original<QueryBoolVec<V>>(name("??8%s@Toshi@@QBE_NABV01@@Z"));
	const auto notEqual = Original<QueryBoolVec<V>>(name("??9%s@Toshi@@QBE_NABV01@@Z"));
	const auto isNormalised = Original<QueryBoolFloat<V>>(name("?IsNormalised@%s@Toshi@@QBE_NM@Z"));
	InputGenerator gen;
	for (int i = 0; i < kIterations; ++i)
	{
		const V a = MakeRandom<V>(gen);
		const V b = (i % 4 == 0) ? a : MakeRandom<V>(gen);
		V unitish = MakeRandom<V>(gen);
		unitish.Normalise();
		const float tolerance = gen.NextFloat(0.0f, 0.001f);
		INFO("a=(", ToString(a), ") b=(", ToString(b), ")");
		CHECK(Same(magnitude(&a), a.Magnitude()));
		CHECK(Same(magnitudeSq(&a), a.MagnitudeSq()));
		CHECK(Same(dotMember(&a, &b), a.DotProduct(b)));
		CHECK(Same(dotStatic(&a, &b), V::DotProduct(a, b)));
		CHECK(Same(distance(&a, &b), V::Distance(a, b)));
		CHECK(Same(distanceSq(&a, &b), V::DistanceSq(a, b)));
		CHECK(isEqual(&a, &b) == a.IsEqual(b));
		CHECK(equal(&a, &b) == (a == b));
		CHECK(notEqual(&a, &b) == (a != b));
		CHECK(isNormalised(&unitish, tolerance) == unitish.IsNormalised(tolerance));
		CHECK(isNormalised(&a, tolerance) == a.IsNormalised(tolerance));
	}
}

template <typename V>
void CheckStaticConstants(const char* cls, std::initializer_list<std::pair<const char*, const V*>> constants)
{
	for (const auto& [member, ours] : constants)
	{
		char mangled[96];
		std::snprintf(mangled, sizeof(mangled), "?%s@%s@Toshi@@2V12@B", member, cls);
		const auto* original = static_cast<const V*>(OriginalModule::Kernel().Find(mangled));
		REQUIRE_MESSAGE(original != nullptr, "export not found: " << mangled);
		INFO(mangled, ": original=(", ToString(*original), ") ours=(", ToString(*ours), ")");
		CHECK(Same(*original, *ours));
	}
}

} // namespace

TEST_SUITE("TVector differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("TVector2 common members")
	{
		GamePrecisionScope precision;
		CheckCommonMembers<TVector2>("TVector2");
	}

	TEST_CASE("TVector3 common members")
	{
		GamePrecisionScope precision;
		CheckCommonMembers<TVector3>("TVector3");
	}

	TEST_CASE("TVector2 specific members")
	{
		GamePrecisionScope precision;
		auto rand = [](InputGenerator& g) { return Random2(g); };
		{
			const auto fn = Original<WithFloat<TVector2>>("?Rotate@TVector2@Toshi@@QAEXM@Z");
			CheckMutator<TVector2>("Rotate", rand,
			                       [&](InputGenerator& g, TVector2& o, TVector2& m)
			                       {
				                       const float angle = g.NextFloat(-10.0f, 10.0f);
				                       fn(&o, angle);
				                       m.Rotate(angle);
			                       });
		}
		{
			const auto fn = Original<Unary<TVector2>>("?SetPerpendicular@TVector2@Toshi@@QAEXXZ");
			CheckMutator<TVector2>("SetPerpendicular", rand,
			                       [&](InputGenerator&, TVector2& o, TVector2& m)
			                       {
				                       fn(&o);
				                       m.SetPerpendicular();
			                       });
		}
		{
			using FromV3 = Method<void, const TVector3*>;
			const auto xy = Original<FromV3>("?SetXYFromVector3@TVector2@Toshi@@QAEXABVTVector3@2@@Z");
			const auto xz = Original<FromV3>("?SetXZFromVector3@TVector2@Toshi@@QAEXABVTVector3@2@@Z");
			CheckMutator<TVector2>("SetXY/XZFromVector3", rand,
			                       [&](InputGenerator& g, TVector2& o, TVector2& m)
			                       {
				                       const TVector3 v = Random3(g);
				                       if (g.NextU32() & 1)
				                       {
					                       xy(&o, &v);
					                       m.SetXYFromVector3(v);
				                       }
				                       else
				                       {
					                       xz(&o, &v);
					                       m.SetXZFromVector3(v);
				                       }
			                       });
		}

		const auto cross = Original<QueryFloatVec<TVector2>>("?CrossProduct@TVector2@Toshi@@QBEMABV12@@Z");
		const auto crossStatic = Original<StaticFloatVecVec<TVector2>>("?CrossProduct@TVector2@Toshi@@SGMABV12@0@Z");
		using ToV3 = Method<TVector3*, TVector3*>;
		const auto getXY = Original<ToV3>("?GetVector3XY@TVector2@Toshi@@QBE?AVTVector3@2@XZ");
		const auto getXZ = Original<ToV3>("?GetVector3XZ@TVector2@Toshi@@QBE?AVTVector3@2@XZ");
		using XZFn = void(__stdcall*)(TVector3*, const TVector3*, const TVector2*);
		const auto addXZ = Original<XZFn>("?AddXZ@TVector2@Toshi@@SGXAAVTVector3@2@ABV32@ABV12@@Z");
		const auto subXZ = Original<XZFn>("?SubXZ@TVector2@Toshi@@SGXAAVTVector3@2@ABV32@ABV12@@Z");
		InputGenerator gen;
		for (int i = 0; i < kIterations; ++i)
		{
			const TVector2 a = Random2(gen), b = Random2(gen);
			const TVector3 v3 = Random3(gen);
			CHECK(Same(cross(&a, &b), a.CrossProduct(b)));
			CHECK(Same(crossStatic(&a, &b), TVector2::CrossProduct(a, b)));
			TVector3 original;
			getXY(&a, &original);
			CHECK(Same(original, a.GetVector3XY()));
			getXZ(&a, &original);
			CHECK(Same(original, a.GetVector3XZ()));
			TVector3 ours;
			addXZ(&original, &v3, &a);
			TVector2::AddXZ(ours, v3, a);
			CHECK(Same(original, ours));
			subXZ(&original, &v3, &a);
			TVector2::SubXZ(ours, v3, a);
			CHECK(Same(original, ours));
		}

		CheckStaticConstants<TVector2>("TVector2", {{"VEC_ZERO", &TVector2::VEC_ZERO},
		                                            {"VEC_POSX", &TVector2::VEC_POSX},
		                                            {"VEC_POSY", &TVector2::VEC_POSY},
		                                            {"VEC_NEGX", &TVector2::VEC_NEGX},
		                                            {"VEC_NEGY", &TVector2::VEC_NEGY}});
	}

	TEST_CASE("TVector3 specific members")
	{
		GamePrecisionScope precision;
		auto rand = [](InputGenerator& g) { return Random3(g); };
		const struct
		{
			const char* mangled;
			void (TVector3::*ours)(float);
		} rotations[] = {
		    {"?RotateX@TVector3@Toshi@@QAEXM@Z", &TVector3::RotateX},
		    {"?RotateY@TVector3@Toshi@@QAEXM@Z", &TVector3::RotateY},
		    {"?RotateZ@TVector3@Toshi@@QAEXM@Z", &TVector3::RotateZ},
		};
		for (const auto& r : rotations)
		{
			const auto fn = Original<WithFloat<TVector3>>(r.mangled);
			CheckMutator<TVector3>(r.mangled, rand,
			                       [&](InputGenerator& g, TVector3& o, TVector3& m)
			                       {
				                       const float angle = g.NextFloat(-10.0f, 10.0f);
				                       fn(&o, angle);
				                       (m.*r.ours)(angle);
			                       });
		}
		{
			const auto fn = Original<WithTwoVecs<TVector3>>("?CrossProduct@TVector3@Toshi@@QAEXABV12@0@Z");
			CheckMutator<TVector3>("CrossProduct", rand,
			                       [&](InputGenerator& g, TVector3& o, TVector3& m)
			                       {
				                       const TVector3 a = Random3(g), b = Random3(g);
				                       fn(&o, &a, &b);
				                       m.CrossProduct(a, b);
			                       });
		}
		{
			const auto fn = Original<WithVecFloat<TVector3>>("?Lerp@TVector3@Toshi@@QAEXABV12@M@Z");
			CheckMutator<TVector3>("Lerp(to,t)", rand,
			                       [&](InputGenerator& g, TVector3& o, TVector3& m)
			                       {
				                       const TVector3 to = Random3(g);
				                       const float t = g.NextFloat(0.0f, 1.0f);
				                       fn(&o, &to, t);
				                       m.Lerp(to, t);
			                       });
		}
		{
			using ClipFn = Method<void, float, float>;
			const auto fn = Original<ClipFn>("?Clip@TVector3@Toshi@@QAEXMM@Z");
			CheckMutator<TVector3>("Clip", rand,
			                       [&](InputGenerator& g, TVector3& o, TVector3& m)
			                       {
				                       const float limit = g.NextFloat(0.0f, 100.0f);
				                       fn(&o, -limit, limit);
				                       m.Clip(-limit, limit);
			                       });
		}
		{
			using ProjectFn = Method<void, TVector3*>;
			const auto unit = Original<ProjectFn>("?SetUnitVectorProjection@TVector3@Toshi@@QAEXAAV12@@Z");
			const auto any = Original<ProjectFn>("?SetVectorProjection@TVector3@Toshi@@QAEXAAV12@@Z");
			CheckMutator<TVector3>("Set*VectorProjection", rand,
			                       [&](InputGenerator& g, TVector3& o, TVector3& m)
			                       {
				                       TVector3 onto = Random3(g);
				                       if (g.NextU32() & 1)
				                       {
					                       onto.Normalise();
					                       unit(&o, &onto);
					                       m.SetUnitVectorProjection(onto);
				                       }
				                       else
				                       {
					                       any(&o, &onto);
					                       m.SetVectorProjection(onto);
				                       }
			                       });
		}

		using ProjectionFn = Method<float, TVector3*>;
		const auto projectionOnUnit = Original<ProjectionFn>("?GetScalarProjectionOnUnit@TVector3@Toshi@@QAEMAAV12@@Z");
		const auto projection = Original<ProjectionFn>("?GetScalarProjection@TVector3@Toshi@@QAEMAAV12@@Z");
		const auto isReasonable = Original<QueryBoolFloat<TVector3>>("?IsReasonable@TVector3@Toshi@@QBE_NM@Z");
		InputGenerator gen;
		for (int i = 0; i < kIterations; ++i)
		{
			TVector3 a = Random3(gen), b = Random3(gen);
			const float limit = gen.NextFloat(0.0f, 120.0f);
			CHECK(Same(projectionOnUnit(&a, &b), a.GetScalarProjectionOnUnit(b)));
			CHECK(Same(projection(&a, &b), a.GetScalarProjection(b)));
			CHECK(isReasonable(&a, limit) == a.IsReasonable(limit));
		}

		CheckStaticConstants<TVector3>("TVector3", {{"VEC_ZERO", &TVector3::VEC_ZERO},
		                                            {"VEC_POSX", &TVector3::VEC_POSX},
		                                            {"VEC_POSY", &TVector3::VEC_POSY},
		                                            {"VEC_POSZ", &TVector3::VEC_POSZ},
		                                            {"VEC_NEGX", &TVector3::VEC_NEGX},
		                                            {"VEC_NEGY", &TVector3::VEC_NEGY},
		                                            {"VEC_NEGZ", &TVector3::VEC_NEGZ},
		                                            {"VEC_POSXPOSZ", &TVector3::VEC_POSXPOSZ},
		                                            {"VEC_POSXNEGZ", &TVector3::VEC_POSXNEGZ},
		                                            {"VEC_NEGXPOSZ", &TVector3::VEC_NEGXPOSZ},
		                                            {"VEC_NEGXNEGZ", &TVector3::VEC_NEGXNEGZ}});
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
