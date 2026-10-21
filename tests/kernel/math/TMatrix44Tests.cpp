#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/math/TMath.h"
#include "toshi/kernel/math/TMatrix44.h"
#include "toshi/kernel/math/TPlane.h"
#include "toshi/kernel/math/TQuaternion.h"
#include "toshi/kernel/math/TVector3.h"
#include "toshi/kernel/math/TVector4.h"

#include <cstring>
#include <doctest/doctest.h>

using Toshi::TMath;
using Toshi::TMatrix44;
using Toshi::TPlane;
using Toshi::TQuaternion;
using Toshi::TVector3;
using Toshi::TVector4;
using namespace jpog::test;

namespace
{

using jpog::test::Same;

bool SameFloats(const float* a, const float* b, int count)
{
	for (int i = 0; i < count; ++i)
		if (!Same(a[i], b[i]))
			return false;
	return true;
}

template <typename T> bool SameAs(const T& a, const T& b)
{
	return SameFloats(reinterpret_cast<const float*>(&a), reinterpret_cast<const float*>(&b), sizeof(T) / 4);
}

doctest::String ToString(const TMatrix44& matrix)
{
	doctest::String text;
	for (int i = 0; i < 16; ++i)
		text += doctest::toString(matrix.m[i / 4][i % 4]) + (i % 4 == 3 ? "; " : ", ");
	return text;
}

TVector3 RandomVec3(InputGenerator& gen, float range = 10.0f)
{
	return TVector3(gen.NextFloat(-range, range), gen.NextFloat(-range, range), gen.NextFloat(-range, range));
}

TVector4 RandomVec4(InputGenerator& gen, float range = 10.0f)
{
	return TVector4(gen.NextFloat(-range, range), gen.NextFloat(-range, range), gen.NextFloat(-range, range),
	                gen.NextFloat(-range, range));
}

TQuaternion RandomRotation(InputGenerator& gen)
{
	TQuaternion q(gen.NextFloat(-1.0f, 1.0f), gen.NextFloat(-1.0f, 1.0f), gen.NextFloat(-1.0f, 1.0f),
	              gen.NextFloat(-1.0f, 1.0f));
	q.Normalise();
	return q;
}

// A mix of general, rigid (rotation + translation), scaled, singular and identity matrices.
TMatrix44 RandomMatrix(InputGenerator& gen)
{
	TMatrix44 matrix;
	switch (gen.NextU32() % 6)
	{
	case 0:
	case 1:
		for (auto& row : matrix.m)
			for (float& value : row)
				value = gen.NextFloat(-2.0f, 2.0f);
		break;
	case 2:
	case 3:
		matrix.SetFromQuaternion(RandomRotation(gen));
		matrix.SetTranslate(RandomVec3(gen));
		if (gen.NextU32() & 1)
			matrix.Scale(gen.NextFloat(-3.0f, 3.0f), gen.NextFloat(0.1f, 3.0f), gen.NextFloat(0.1f, 3.0f));
		break;
	case 4:
		for (auto& row : matrix.m)
			for (float& value : row)
				value = gen.NextFloat(-2.0f, 2.0f);
		std::memcpy(matrix.m[gen.NextU32() % 3], matrix.m[3], sizeof(matrix.m[3])); // singular
		break;
	default:
		matrix = TMatrix44::IDENTITY;
		break;
	}
	return matrix;
}

TPlane RandomPlane(InputGenerator& gen)
{
	TVector3 normal = RandomVec3(gen);
	normal.Normalise();
	return TPlane(normal, gen.NextFloat(-10.0f, 10.0f));
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TMatrix44")
{
	TEST_CASE("inverse and transforms")
	{
		TMatrix44 matrix;
		matrix.SetFromQuaternion(TQuaternion(TVector3::VEC_POSZ, TMath::HALF_PI));
		matrix.SetTranslate(1.0f, 2.0f, 3.0f);
		TMatrix44 inverse;
		REQUIRE(inverse.Invert4(matrix));
		const TMatrix44 product = matrix * inverse;
		for (int i = 0; i < 16; ++i)
			CHECK(product.m[i / 4][i % 4] == doctest::Approx(TMatrix44::IDENTITY.m[i / 4][i % 4]).epsilon(1e-5));
		CHECK(matrix.IsOrthonormal());
		CHECK_FALSE(matrix.IsReflected());

		TVector3 point;
		TMatrix44::TransformVector(point, matrix, TVector3::VEC_POSX);
		CHECK(point.x == doctest::Approx(1.0f));
		CHECK(point.y == doctest::Approx(3.0f));
		CHECK(point.z == doctest::Approx(3.0f));

		TQuaternion back;
		TMatrix44::MatToQuat(back, matrix);
		CHECK(std::fabs(TVector4::DotProduct4(back.AsVector4(),
		                                      TQuaternion(TVector3::VEC_POSZ, TMath::HALF_PI).AsVector4())) ==
		      doctest::Approx(1.0f));
	}

	TEST_CASE("TPlane")
	{
		const TPlane plane(TVector3::VEC_POSY, TVector3(4.0f, 5.0f, 6.0f));
		CHECK(plane.GetD() == 5.0f);
		TVector3 point;
		plane.GetPoint(point);
		CHECK(point == TVector3(0.0f, 5.0f, 0.0f));
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

namespace
{

constexpr int kIterations = 2000;
using M = const TMatrix44*;

// Runs `call` on identical copies of a random matrix and compares the matrices afterwards
// (`call` checks any other outputs itself).
template <typename Call> void CheckMutator(const char* name, Call call)
{
	InputGenerator gen;
	for (int i = 0; i < kIterations; ++i)
	{
		TMatrix44 original = RandomMatrix(gen);
		TMatrix44 ours = original;
		const TMatrix44 before = original;
		call(gen, original, ours);
		INFO(doctest::String(name), ": self=(", ToString(before), ")\n original=(", ToString(original), ")\n ours=(",
		     ToString(ours), ")");
		CHECK(SameAs(original, ours));
	}
}

} // namespace

TEST_SUITE("TMatrix44 differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("mutators")
	{
		GamePrecisionScope precision;
		const struct
		{
			const char* mangled;
			void (TMatrix44::*ours)();
		} unary[] = {
		    {"?Identity@TMatrix44@Toshi@@QAEXXZ", &TMatrix44::Identity},
		    {"?InvertOrthogonal@TMatrix44@Toshi@@QAEXXZ", &TMatrix44::InvertOrthogonal},
		};
		for (const auto& c : unary)
		{
			const auto fn = Original<Method<void>>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator&, TMatrix44& o, TMatrix44& m)
			             {
				             fn(&o);
				             (m.*c.ours)();
			             });
		}
		{
			const auto fn = Original<Method<bool>>("?Invert@TMatrix44@Toshi@@QAE_NXZ");
			CheckMutator("Invert()", [&](InputGenerator&, TMatrix44& o, TMatrix44& m) { CHECK(fn(&o) == m.Invert()); });
		}

		const struct
		{
			const char* mangled;
			void (TMatrix44::*ours)(const TMatrix44&);
		} withMatrix[] = {
		    {"?Add@TMatrix44@Toshi@@QAEXABV12@@Z", &TMatrix44::Add},
		    {"?Subtract@TMatrix44@Toshi@@QAEXABV12@@Z", &TMatrix44::Subtract},
		    {"?Multiply@TMatrix44@Toshi@@QAEXABV12@@Z", &TMatrix44::Multiply},
		    {"?Transpose@TMatrix44@Toshi@@QAEXABV12@@Z", &TMatrix44::Transpose},
		    {"?InvertOrthogonal@TMatrix44@Toshi@@QAEXABV12@@Z", &TMatrix44::InvertOrthogonal},
		};
		for (const auto& c : withMatrix)
		{
			const auto fn = Original<Method<void, M>>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TMatrix44& o, TMatrix44& m)
			             {
				             const TMatrix44 arg = RandomMatrix(g);
				             fn(&o, &arg);
				             (m.*c.ours)(arg);
			             });
		}
		const struct
		{
			const char* mangled;
			bool (TMatrix44::*ours)(const TMatrix44&);
		} inverses[] = {
		    {"?Invert@TMatrix44@Toshi@@QAE_NABV12@@Z", &TMatrix44::Invert},
		    {"?Invert4@TMatrix44@Toshi@@QAE_NABV12@@Z", &TMatrix44::Invert4},
		    {"?InvertArbitrary@TMatrix44@Toshi@@QAE_NABV12@@Z", &TMatrix44::InvertArbitrary},
		};
		for (const auto& c : inverses)
		{
			const auto fn = Original<Method<bool, M>>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TMatrix44& o, TMatrix44& m)
			             {
				             const TMatrix44 arg = RandomMatrix(g);
				             CHECK(fn(&o, &arg) == (m.*c.ours)(arg));
			             });
		}
		const struct
		{
			const char* mangled;
			void (TMatrix44::*ours)(const TMatrix44&, const TMatrix44&);
		} withTwo[] = {
		    {"?Add@TMatrix44@Toshi@@QAEXABV12@0@Z", &TMatrix44::Add},
		    {"?Subtract@TMatrix44@Toshi@@QAEXABV12@0@Z", &TMatrix44::Subtract},
		    {"?Multiply@TMatrix44@Toshi@@QAEXABV12@0@Z", &TMatrix44::Multiply},
		};
		for (const auto& c : withTwo)
		{
			const auto fn = Original<Method<void, M, M>>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TMatrix44& o, TMatrix44& m)
			             {
				             const TMatrix44 a = RandomMatrix(g), b = RandomMatrix(g);
				             fn(&o, &a, &b);
				             (m.*c.ours)(a, b);
			             });
		}
		const struct
		{
			const char* mangled;
			void (TMatrix44::*ours)(float);
		} withFloat[] = {
		    {"?Multiply@TMatrix44@Toshi@@QAEXM@Z", &TMatrix44::Multiply},
		    {"?Divide@TMatrix44@Toshi@@QAEXM@Z", &TMatrix44::Divide},
		    {"?Scale@TMatrix44@Toshi@@QAEXM@Z", &TMatrix44::Scale},
		    {"?RotateX@TMatrix44@Toshi@@QAEXM@Z", &TMatrix44::RotateX},
		    {"?RotateY@TMatrix44@Toshi@@QAEXM@Z", &TMatrix44::RotateY},
		    {"?RotateZ@TMatrix44@Toshi@@QAEXM@Z", &TMatrix44::RotateZ},
		};
		for (const auto& c : withFloat)
		{
			const auto fn = Original<Method<void, float>>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TMatrix44& o, TMatrix44& m)
			             {
				             const float s = g.NextFloat(-7.0f, 7.0f);
				             fn(&o, s);
				             (m.*c.ours)(s);
			             });
		}
		const struct
		{
			const char* mangled;
			void (TMatrix44::*ours)(const TMatrix44&, float);
		} withMatrixFloat[] = {
		    {"?Multiply@TMatrix44@Toshi@@QAEXABV12@M@Z", &TMatrix44::Multiply},
		    {"?Divide@TMatrix44@Toshi@@QAEXABV12@M@Z", &TMatrix44::Divide},
		};
		for (const auto& c : withMatrixFloat)
		{
			const auto fn = Original<Method<void, M, float>>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TMatrix44& o, TMatrix44& m)
			             {
				             const TMatrix44 arg = RandomMatrix(g);
				             const float s = g.NextFloat(-7.0f, 7.0f);
				             fn(&o, &arg, s);
				             (m.*c.ours)(arg, s);
			             });
		}
		const struct
		{
			const char* mangled;
			void (TMatrix44::*ours)(float, float, float);
		} withThree[] = {
		    {"?Scale@TMatrix44@Toshi@@QAEXMMM@Z", &TMatrix44::Scale},
		    {"?Translate@TMatrix44@Toshi@@QAEXMMM@Z", &TMatrix44::Translate},
		    {"?Rotate@TMatrix44@Toshi@@QAEXMMM@Z", &TMatrix44::Rotate},
		    {"?SetTranslate@TMatrix44@Toshi@@QAEXMMM@Z", &TMatrix44::SetTranslate},
		};
		for (const auto& c : withThree)
		{
			const auto fn = Original<Method<void, float, float, float>>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TMatrix44& o, TMatrix44& m)
			             {
				             const TVector3 v = RandomVec3(g);
				             fn(&o, v.x, v.y, v.z);
				             (m.*c.ours)(v.x, v.y, v.z);
			             });
		}
		const struct
		{
			const char* mangled;
			void (TMatrix44::*ours)(const TVector3&);
		} withVec3[] = {
		    {"?Translate@TMatrix44@Toshi@@QAEXABVTVector3@2@@Z", &TMatrix44::Translate},
		    {"?SetTranslate@TMatrix44@Toshi@@QAEXABVTVector3@2@@Z", &TMatrix44::SetTranslate},
		};
		for (const auto& c : withVec3)
		{
			const auto fn = Original<Method<void, const TVector3*>>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TMatrix44& o, TMatrix44& m)
			             {
				             const TVector3 v = RandomVec3(g);
				             fn(&o, &v);
				             (m.*c.ours)(v);
			             });
		}
		const struct
		{
			const char* mangled;
			void (TMatrix44::*ours)(const TVector4&);
		} withVec4[] = {
		    {"?Translate@TMatrix44@Toshi@@QAEXABVTVector4@2@@Z", &TMatrix44::Translate},
		    {"?SetTranslate@TMatrix44@Toshi@@QAEXABVTVector4@2@@Z", &TMatrix44::SetTranslate},
		    {"?Scale@TMatrix44@Toshi@@QAEXABVTVector4@2@@Z", &TMatrix44::Scale},
		};
		for (const auto& c : withVec4)
		{
			const auto fn = Original<Method<void, const TVector4*>>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TMatrix44& o, TMatrix44& m)
			             {
				             const TVector4 v = RandomVec4(g);
				             fn(&o, &v);
				             (m.*c.ours)(v);
			             });
		}

		const auto fromQuat = Original<Method<TMatrix44*, const TQuaternion*>>(
		    "?SetFromQuaternion@TMatrix44@Toshi@@QAEAAV12@ABVTQuaternion@2@@Z");
		const auto push = Original<Method<TMatrix44*, const TQuaternion*, M, const TVector3*>>(
		    "?PushQuaternion@TMatrix44@Toshi@@QAEAAV12@ABVTQuaternion@2@ABV12@ABVTVector3@2@@Z");
		const auto lookAt2 =
		    Original<Method<void, TVector3*, TVector3*>>("?LookAtTarget@TMatrix44@Toshi@@QAEXAAVTVector3@2@0@Z");
		const auto lookAt1 = Original<Method<void, TVector3*>>("?LookAtTarget@TMatrix44@Toshi@@QAEXAAVTVector3@2@@Z");
		CheckMutator("SetFromQuaternion/PushQuaternion/LookAtTarget",
		             [&](InputGenerator& g, TMatrix44& o, TMatrix44& m)
		             {
			             TQuaternion q = RandomRotation(g);
			             if (g.NextU32() % 4 == 0)
				             q.x *= 1.5f; // not normalised
			             const TMatrix44 arg = RandomMatrix(g);
			             TVector3 target = RandomVec3(g), eye = RandomVec3(g);
			             switch (g.NextU32() % 4)
			             {
			             case 0:
				             CHECK(fromQuat(&o, &q) == &o);
				             m.SetFromQuaternion(q);
				             break;
			             case 1:
				             CHECK(push(&o, &q, &arg, &target) == &o);
				             m.PushQuaternion(q, arg, target);
				             break;
			             case 2:
				             lookAt2(&o, &target, &eye);
				             m.LookAtTarget(target, eye);
				             break;
			             default:
				             lookAt1(&o, &target);
				             m.LookAtTarget(target);
				             break;
			             }
		             });
	}

	TEST_CASE("operators")
	{
		GamePrecisionScope precision;
		const struct
		{
			const char* mangled;
			TMatrix44& (TMatrix44::*ours)(const TMatrix44&);
		} compound[] = {{"??YTMatrix44@Toshi@@QAEAAV01@ABV01@@Z", &TMatrix44::operator+= },
		                 {"??ZTMatrix44@Toshi@@QAEAAV01@ABV01@@Z", &TMatrix44::operator-= },
		                  { "??XTMatrix44@Toshi@@QAEAAV01@ABV01@@Z",
			                &TMatrix44::operator*= },
		                 };
		for (const auto& c : compound)
		{
			const auto fn = Original<Method<TMatrix44*, M>>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TMatrix44& o, TMatrix44& m)
			             {
				             const TMatrix44 arg = RandomMatrix(g);
				             CHECK(fn(&o, &arg) == &o);
				             (m.*c.ours)(arg);
			             });
		}
		const struct
		{
			const char* mangled;
			TMatrix44& (TMatrix44::*ours)(float);
		} compoundFloat[] = {{"??XTMatrix44@Toshi@@QAEAAV01@M@Z", &TMatrix44::operator*= },
		                      { "??_0TMatrix44@Toshi@@QAEAAV01@M@Z",
			                    &TMatrix44::operator/= },
		};
		for (const auto& c : compoundFloat)
		{
			const auto fn = Original<Method<TMatrix44*, float>>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TMatrix44& o, TMatrix44& m)
			             {
				             const float s = g.NextFloat(-7.0f, 7.0f);
				             CHECK(fn(&o, s) == &o);
				             (m.*c.ours)(s);
			             });
		}
		const struct
		{
			const char* mangled;
			TMatrix44 (TMatrix44::*ours)(const TMatrix44&) const;
		} value[] = {{"??HTMatrix44@Toshi@@QBE?AV01@ABV01@@Z", &TMatrix44::operator+ },
		              {"??GTMatrix44@Toshi@@QBE?AV01@ABV01@@Z", &TMatrix44::operator- },
		               { "??DTMatrix44@Toshi@@QBE?AV01@ABV01@@Z",
			             &TMatrix44::operator* },
		              };
		for (const auto& c : value)
		{
			const auto fn = Original<Method<TMatrix44*, TMatrix44*, M>>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TMatrix44& o, TMatrix44& m)
			             {
				             const TMatrix44 lhs = RandomMatrix(g), rhs = RandomMatrix(g);
				             CHECK(fn(&lhs, &o, &rhs) == &o);
				             m = (lhs.*c.ours)(rhs);
			             });
		}
		const struct
		{
			const char* mangled;
			TMatrix44 (TMatrix44::*ours)(float) const;
		} valueFloat[] = {{"??DTMatrix44@Toshi@@QBE?AV01@M@Z", &TMatrix44::operator* },
		                   { "??KTMatrix44@Toshi@@QBE?AV01@M@Z",
			                 &TMatrix44::operator/ },
		};
		for (const auto& c : valueFloat)
		{
			const auto fn = Original<Method<TMatrix44*, TMatrix44*, float>>(c.mangled);
			CheckMutator(c.mangled,
			             [&](InputGenerator& g, TMatrix44& o, TMatrix44& m)
			             {
				             const TMatrix44 lhs = RandomMatrix(g);
				             const float s = g.NextFloat(-7.0f, 7.0f);
				             CHECK(fn(&lhs, &o, s) == &o);
				             m = (lhs.*c.ours)(s);
			             });
		}
	}

	TEST_CASE("queries, transforms and constants")
	{
		GamePrecisionScope precision;
		const auto det = Original<Method<float>>("?Determinant@TMatrix44@Toshi@@QBEMXZ");
		const auto det4 = Original<Method<float>>("?Determinant4@TMatrix44@Toshi@@QBEMXZ");
		const auto isEqual = Original<Method<bool, M>>("?IsEqual@TMatrix44@Toshi@@QBE_NABV12@@Z");
		const auto equal = Original<Method<bool, M>>("??8TMatrix44@Toshi@@QBE_NABV01@@Z");
		const auto notEqual = Original<Method<bool, M>>("??9TMatrix44@Toshi@@QBE_NABV01@@Z");
		const auto isIdentity = Original<Method<bool>>("?IsIdentity@TMatrix44@Toshi@@QBE_NXZ");
		const auto isOrthogonal = Original<Method<bool>>("?IsOrthogonal@TMatrix44@Toshi@@QBE_NXZ");
		const auto isOrthonormal = Original<Method<bool>>("?IsOrthonormal@TMatrix44@Toshi@@QBE_NXZ");
		const auto isReflected = Original<Method<bool>>("?IsReflected@TMatrix44@Toshi@@QBE_NXZ");
		using V3Fn = void(__stdcall*)(TVector3*, M, const TVector3*);
		using V4Fn = void(__stdcall*)(TVector4*, M, const TVector4*);
		using PlaneFn = void(__stdcall*)(TPlane*, M, const TPlane*);
		const auto transform3 = Original<V3Fn>("?TransformVector@TMatrix44@Toshi@@SGXAAVTVector3@2@ABV12@ABV32@@Z");
		const auto transform4 = Original<V4Fn>("?TransformVector@TMatrix44@Toshi@@SGXAAVTVector4@2@ABV12@ABV32@@Z");
		const auto rotate3 = Original<V3Fn>("?RotateVector@TMatrix44@Toshi@@SGXAAVTVector3@2@ABV12@ABV32@@Z");
		const auto rotate4 = Original<V4Fn>("?RotateVector@TMatrix44@Toshi@@SGXAAVTVector4@2@ABV12@ABV32@@Z");
		const auto plane = Original<PlaneFn>("?TransformPlane@TMatrix44@Toshi@@SGXAAVTPlane@2@ABV12@ABV32@@Z");
		const auto planeOrtho =
		    Original<PlaneFn>("?TransformPlaneOrthogonal@TMatrix44@Toshi@@SGXAAVTPlane@2@ABV12@ABV32@@Z");
		const auto matToQuat =
		    Original<void(__stdcall*)(TQuaternion*, M)>("?MatToQuat@TMatrix44@Toshi@@SGXAAVTQuaternion@2@ABV12@@Z");
		const auto euler = Original<Method<void, float*>>("?GetEulerXYZ@TQuaternion@Toshi@@QBEXPAM@Z");
		const auto mulV3 =
		    Original<Method<TVector3*, TVector3*, const TVector3*>>("??DTMatrix44@Toshi@@QBE?AVTVector3@1@ABV21@@Z");
		const auto mulV4 =
		    Original<Method<TVector4*, TVector4*, const TVector4*>>("??DTMatrix44@Toshi@@QBE?AVTVector4@1@ABV21@@Z");
		const auto mulPlane =
		    Original<Method<TPlane*, TPlane*, const TPlane*>>("??DTMatrix44@Toshi@@QBE?AVTPlane@1@ABV21@@Z");

		InputGenerator gen;
		for (int i = 0; i < kIterations; ++i)
		{
			const TMatrix44 a = RandomMatrix(gen);
			const TMatrix44 b = (i % 4 == 0) ? a : RandomMatrix(gen);
			const TVector3 v3 = RandomVec3(gen);
			const TVector4 v4 = RandomVec4(gen);
			const TPlane p = RandomPlane(gen);
			INFO("a=(", ToString(a), ")");
			CHECK(Same(det(&a), a.Determinant()));
			CHECK(Same(det4(&a), a.Determinant4()));
			CHECK(isEqual(&a, &b) == a.IsEqual(b));
			CHECK(equal(&a, &b) == (a == b));
			CHECK(notEqual(&a, &b) == (a != b));
			CHECK(isIdentity(&a) == a.IsIdentity());
			CHECK(isOrthogonal(&a) == a.IsOrthogonal());
			CHECK(isOrthonormal(&a) == a.IsOrthonormal());
			CHECK(isReflected(&a) == a.IsReflected());

			TVector3 o3, m3;
			transform3(&o3, &a, &v3);
			TMatrix44::TransformVector(m3, a, v3);
			CHECK(SameAs(o3, m3));
			rotate3(&o3, &a, &v3);
			TMatrix44::RotateVector(m3, a, v3);
			CHECK(SameAs(o3, m3));
			CHECK(mulV3(&a, &o3, &v3) == &o3);
			CHECK(SameAs(o3, a * v3));
			TVector4 o4, m4;
			transform4(&o4, &a, &v4);
			TMatrix44::TransformVector(m4, a, v4);
			CHECK(SameAs(o4, m4));
			rotate4(&o4, &a, &v4);
			TMatrix44::RotateVector(m4, a, v4);
			CHECK(SameAs(o4, m4));
			CHECK(mulV4(&a, &o4, &v4) == &o4);
			CHECK(SameAs(o4, a * v4));
			TPlane op, mp;
			plane(&op, &a, &p);
			TMatrix44::TransformPlane(mp, a, p);
			CHECK(SameAs(op, mp));
			planeOrtho(&op, &a, &p);
			TMatrix44::TransformPlaneOrthogonal(mp, a, p);
			CHECK(SameAs(op, mp));
			CHECK(mulPlane(&a, &op, &p) == &op);
			CHECK(SameAs(op, a * p));

			// MatToQuat on rotations (every branch: w, x, y or z largest) and on general matrices.
			TMatrix44 rotation;
			rotation.SetFromQuaternion(RandomRotation(gen));
			const TMatrix44& source = (i & 1) ? rotation : a;
			TQuaternion oq, mq;
			matToQuat(&oq, &source);
			TMatrix44::MatToQuat(mq, source);
			CHECK(SameAs(oq, mq));

			TQuaternion q = RandomRotation(gen);
			if (i % 8 == 0)
				q.Set(0.0f, (i & 16) ? 0.70710677f : -0.70710677f, 0.0f, 0.70710677f); // gimbal lock
			float oe[3], me[3];
			euler(&q, oe);
			q.GetEulerXYZ(me);
			CHECK(SameFloats(oe, me, 3));
		}

		const auto* identity =
		    static_cast<const TMatrix44*>(OriginalModule::Kernel().Find("?IDENTITY@TMatrix44@Toshi@@2V12@B"));
		const auto* deviation =
		    static_cast<const float*>(OriginalModule::Kernel().Find("?MAX_ORTHODEVIATION@TMatrix44@Toshi@@2MB"));
		const auto* scalar =
		    static_cast<const float*>(OriginalModule::Kernel().Find("?QUATSCALAR@TMatrix44@Toshi@@2MB"));
		REQUIRE(identity != nullptr);
		REQUIRE(deviation != nullptr);
		REQUIRE(scalar != nullptr);
		CHECK(SameAs(*identity, TMatrix44::IDENTITY));
		CHECK(Same(*deviation, TMatrix44::MAX_ORTHODEVIATION));
		CHECK(Same(*scalar, TMatrix44::QUATSCALAR));
	}

	TEST_CASE("TPlane")
	{
		GamePrecisionScope precision;
		using V3 = const TVector3*;
		const auto setPoints = Original<Method<void, V3, V3>>("?Set@TPlane@Toshi@@QAEXABVTVector3@2@0@Z");
		const auto construct = Original<Method<void, V3, V3>>("??0TPlane@Toshi@@QAE@ABVTVector3@1@0@Z");
		const auto setPoint = Original<Method<void, V3>>("?SetPoint@TPlane@Toshi@@QAEXABVTVector3@2@@Z");
		const auto getPoint = Original<Method<void, TVector3*>>("?GetPoint@TPlane@Toshi@@QBEXAAVTVector3@2@@Z");
		const auto negate = Original<Method<void, const TPlane*>>("?Negate@TPlane@Toshi@@QAEXABV12@@Z");
		const auto getNormal = Original<Method<const TVector3*>>("?GetNormal@TPlane@Toshi@@QBEABVTVector3@2@XZ");
		InputGenerator gen;
		for (int i = 0; i < kIterations; ++i)
		{
			const TVector3 normal = RandomVec3(gen), point = RandomVec3(gen);
			const TPlane source = RandomPlane(gen);
			TPlane o = source, m = source;
			setPoints(&o, &normal, &point);
			m.Set(normal, point);
			CHECK(SameAs(o, m));
			construct(&o, &normal, &point);
			CHECK(SameAs(o, TPlane(normal, point)));
			setPoint(&o, &point);
			m.SetPoint(point);
			CHECK(SameAs(o, m));
			TVector3 op, mp;
			getPoint(&source, &op);
			source.GetPoint(mp);
			CHECK(SameAs(op, mp));
			negate(&o, &source);
			m.Negate(source);
			CHECK(SameAs(o, m));
			CHECK(getNormal(&source) == &source.GetNormal());
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
