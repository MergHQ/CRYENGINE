// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <CrySerialization/Forward.h>
#include <CryMath/Cry_Math.h>
#include <CryMath/Random.h>
#include <Schematyc/Types/MathTypes.h>

#include "AutoRegister.h"
#include "STDModules.h"

namespace Schematyc
{
namespace MathUtils
{

inline Matrix33 CreateOrthonormalTM33(Vec3 at, Vec3 up = Vec3(0.0f, 0.0f, 1.0f))
{
	SCHEMATYC_ENV_ASSERT(at.IsUnit())
	SCHEMATYC_ENV_ASSERT(up.IsUnit());
	const Vec3 right = at.Cross(up).GetNormalized();
	up = right.Cross(at).GetNormalized();
	return Matrix33::CreateFromVectors(right, at, up);
}

} // MathUtils

namespace Float
{

float Add(float a, float b)
{
	return a + b;
}

float Subtract(float a, float b)
{
	return a - b;
}

float Multiply(float a, float b)
{
	return a * b;
}

float Clamp(float value, float min, float max)
{
	return crymath::clamp(value, min, max);
}

float Divide(float a, float b)
{
	return b != 0.0f ? a / b : 0.0f;
}

float Modulus(float a, float b)
{
	return b != 0.0f ? fmodf(a, b) : 0.0f;
}

float Sin(float x)
{
	return sin_tpl(DEG2RAD(x));
}

float ArcSin(float x)
{
	return RAD2DEG(asin_tpl(x));
}

float Cos(float x)
{
	return cos_tpl(DEG2RAD(x));
}

float ArcCos(float x)
{
	return RAD2DEG(acos_tpl(x));
}

float Tan(float x)
{
	return tan_tpl(DEG2RAD(x));
}

float ArcTan(float x)
{
	return RAD2DEG(atan_tpl(x));
}

float Random(float min, float max)
{
	return min < max ? cry_random(min, max) : min;
}

float Abs(float x)
{
	return fabs_tpl(x);
}

bool Compare(float a, float b, EComparisonMode mode)
{
	switch (mode)
	{
	case EComparisonMode::Equal:
		{
			return a == b;
		}
	case EComparisonMode::NotEqual:
		{
			return a != b;
		}
	case EComparisonMode::LessThan:
		{
			return a < b;
		}
	case EComparisonMode::LessThanOrEqual:
		{
			return a <= b;
		}
	case EComparisonMode::GreaterThan:
		{
			return a > b;
		}
	case EComparisonMode::GreaterThanOrEqual:
		{
			return a >= b;
		}
	default:
		{
			SCHEMATYC_ENV_ERROR("Invalid comparison mode!");
			return false;
		}
	}
}

int32 ToInt32(float input)
{
	return static_cast<int32>(input);
}

uint32 ToUInt32(float input)
{
	return static_cast<uint32>(input);
}

void ToString(float input, CSharedString& output)
{
	Schematyc::ToString(output, input);
}

static void RegisterFunctions(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(GetTypeDesc<float>().GetGUID());
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Add, "d62aede8-ed2c-47d9-b7f3-6de5ed0ef51f"_schematyc_guid, "Add");
		pFunction->SetDescription("Add A to B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Subtract, "3a7c8a1d-2938-40f8-b5ac-b6becf32baf2"_schematyc_guid, "Subtract");
		pFunction->SetDescription("Subtract B from A");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Multiply, "b04f428a-4408-4c3d-b73b-173a9738c857"_schematyc_guid, "Multiply");
		pFunction->SetDescription("Multiply A by B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Divide, "fd26901b-dbbe-4f4d-b8c1-6547f55339ae"_schematyc_guid, "Divide");
		pFunction->SetDescription("Divide A by B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{	
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Clamp, "350ad86a-a61a-40c4-9f2e-30b6fdf2ae06"_schematyc_guid, "Clamp");
		pFunction->SetDescription("Clamp the value between min and max");
		pFunction->BindInput(1, 'val', "Value");
		pFunction->BindInput(2, 'min', "Min");
		pFunction->BindInput(3, 'max', "Max");
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Modulus, "fc35a48f-7926-4417-b940-08162bc4a9a5"_schematyc_guid, "Modulus");
		pFunction->SetDescription("Calculate remainder when A is divided by B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Sin, "d21a3090-e17f-42a4-a5e5-2f153adadd8b"_schematyc_guid, "Sin");
		pFunction->SetDescription("Calculate sine of X (degrees)");
		pFunction->BindInput(1, 'x', "X");
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ArcSin, "3898f5c4-fead-4976-87f7-11d902cff5ad"_schematyc_guid, "ArcSin");
		pFunction->SetDescription("Calculate arc-sine of X (degrees)");
		pFunction->BindInput(1, 'x', "X");
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Cos, "0a6e3000-9ccc-4a72-874d-6b0c2546eef4"_schematyc_guid, "Cos");
		pFunction->SetDescription("Calculate cosine of X (degrees)");
		pFunction->BindInput(1, 'x', "X");
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ArcCos, "3df97c26-17f0-45a7-b07d-dae4ddbd84bd"_schematyc_guid, "ArcCos");
		pFunction->SetDescription("Calculate arc-cosine of X (degrees)");
		pFunction->BindInput(1, 'x', "X");
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Tan, "0865c078-a019-472e-8d36-62a68d504edd"_schematyc_guid, "Tan");
		pFunction->SetDescription("Calculate tangent of X (degrees)");
		pFunction->BindInput(1, 'x', "X");
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ArcTan, "7ac8f74c-4ed9-4138-853a-5df14c452efc"_schematyc_guid, "ArcTan");
		pFunction->SetDescription("Calculate arc-tangent of X (degrees)");
		pFunction->BindInput(1, 'x', "X");
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Random, "a17b82b7-3b4a-440d-a4f1-082f3d9cf0f5"_schematyc_guid, "Random");
		pFunction->SetDescription("Generate random number within range");
		pFunction->BindInput(1, 'min', "Min", "Minimum value", 0.0f);
		pFunction->BindInput(2, 'max', "Max", "Minimum value", 100.0f);
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Abs, "08e66eec-9e40-4fe9-b4dd-0df8073c681d"_schematyc_guid, "Abs");
		pFunction->SetDescription("Calculate absolute (non-negative) value");
		pFunction->BindInput(1, 'x', "Input"); // #SchematycTODO : Rename 'X'!
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Compare, "580eb35f-7032-4a87-8c62-eb140c4f9b3e"_schematyc_guid, "Compare");
		pFunction->SetDescription("Compare A and B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindInput(3, 'mode', "Mode", "Comparison mode", EComparisonMode::Equal);
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ToInt32, "6e6356f3-7cf6-4746-9ef7-e8031c2a4117"_schematyc_guid, "ToInt32");  //#SchematycTODO: parameter to define how to round
		pFunction->SetDescription("Convert to Int32");
		pFunction->BindInput(1, 'val', "Input"); // #SchematycTODO : Rename 'Value'!
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ToUInt32, "FAA240D7-7187-4A9F-8345-3D77A095EDE2"_schematyc_guid, "ToUInt32");
		pFunction->SetDescription("Convert to UInt32");
		pFunction->BindInput(1, 'val', "Input"); // #SchematycTODO : Rename 'Value'!
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ToString, "171188d6-b66a-4d4b-8508-b6c003c9da78"_schematyc_guid, "ToString");
		pFunction->SetDescription("Convert to String");
		pFunction->BindInput(1, 'val', "Input"); // #SchematycTODO : Rename 'Value'!
		pFunction->BindOutput(2, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
}

} // Float

namespace Vector2
{

Vec2 Create(float x, float y)
{
	return Vec2(x, y);
}

void Expand(const Vec2& input, float& x, float& y)
{
	x = input.x;
	y = input.y;
}

float Length(const Vec2& input)
{
	return input.GetLength();
}

float LengthSq(const Vec2& input)
{
	return input.GetLengthSquared();
}

float Distance(const Vec2& a, const Vec2& b)
{
	const Vec2 delta = a - b;
	return delta.GetLength();
}

float DistanceSq(const Vec2& a, const Vec2& b)
{
	const Vec2 delta = a - b;
	return delta.GetLengthSquared();
}

Vec2 Normalize(const Vec2& input)
{
	return input.GetNormalized();
}

Vec2 Add(const Vec2& a, const Vec2& b)
{
	return a + b;
}

Vec2 Subtract(const Vec2& a, const Vec2& b)
{
	return a - b;
}

Vec2 Clamp(const Vec2& value, const Vec2& min, const Vec2& max)
{
	return crymath::clamp(value, min, max);
}

Vec2 Scale(const Vec2& input, float scale)
{
	return input * scale;
}

float DotProduct(const Vec2& a, const Vec2& b)
{
	return a.Dot(b);
}

float Angle(const Vec2& a, const Vec2& b)
{
	const float deg = RAD2DEG(atan2(a.y, a.x) - atan2(b.y, b.x));
	return deg > 0.0f ? deg : 360.0f + deg;
}

void ToString(const Vec2& input, CSharedString& output)
{
	Schematyc::ToString(output, input);
}

static void RegisterFunctions(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(GetTypeDesc<Vec2>().GetGUID());
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Create, "19f18792-2347-4385-bd90-433c903260aa"_schematyc_guid, "Create");
		pFunction->SetDescription("Create Vector2");
		pFunction->BindInput(1, 'x', "X");
		pFunction->BindInput(2, 'y', "Y");
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Expand, "e03eebae-2f92-4a1d-b5e3-81d5a69dcabf"_schematyc_guid, "Expand");
		pFunction->SetDescription("Expand Vector2");
		pFunction->BindInput(1, 'val', "Input", nullptr, Vec2(ZERO)); // #SchematycTODO : Rename 'Value'!
		pFunction->BindOutput(2, 'x', "X");
		pFunction->BindOutput(3, 'y', "Y");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Length, "796a20cd-dc27-47c5-99b3-292f16f46e46"_schematyc_guid, "Length");
		pFunction->SetDescription("Calculate length of vector");
		pFunction->BindInput(1, 'val', "Input", nullptr, Vec2(ZERO)); // #SchematycTODO : Rename 'Value'!
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&LengthSq, "85f725fa-0676-47f5-ad2e-f2fa2393fae4"_schematyc_guid, "LengthSq");
		pFunction->SetDescription("Calculate squared length of vector");
		pFunction->BindInput(1, 'val', "Input", nullptr, Vec2(ZERO)); // #SchematycTODO : Rename 'Value'!
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Distance, "6ad3e5fa-715d-4e4a-bd03-ffa5ba7d9d6d"_schematyc_guid, "Distance");
		pFunction->SetDescription("Calculate distance between two points");
		pFunction->BindInput(1, 'a', "A", nullptr, Vec2(ZERO));
		pFunction->BindInput(2, 'b', "B", nullptr, Vec2(ZERO));
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&DistanceSq, "4f626698-3eae-4a73-81e2-eae835d22688"_schematyc_guid, "DistanceSq");
		pFunction->SetDescription("Calculate squared distance between two points");
		pFunction->BindInput(1, 'a', "A", nullptr, Vec2(ZERO));
		pFunction->BindInput(2, 'b', "B", nullptr, Vec2(ZERO));
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Normalize, "7aa8e16e-3318-427d-980e-2213c0fdBd13"_schematyc_guid, "Normalize");
		pFunction->SetDescription("Normalize vector");
		pFunction->BindInput(1, 'val', "Input", nullptr, Vec2(ZERO)); // #SchematycTODO : Rename 'Value'!
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Add, "d1b45d39-bef5-4f18-b7e1-7fad1ea78dd3"_schematyc_guid, "Add");
		pFunction->SetDescription("Add A to B");
		pFunction->BindInput(1, 'b', "A", nullptr, Vec2(ZERO));
		pFunction->BindInput(2, 'a', "B", nullptr, Vec2(ZERO));
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Subtract, "edc23583-23ee-4a1c-b24b-1fbd6a503b03"_schematyc_guid, "Subtract");
		pFunction->SetDescription("Subtract B from A");
		pFunction->BindInput(1, 'a', "A", nullptr, Vec2(ZERO));
		pFunction->BindInput(2, 'b', "B", nullptr, Vec2(ZERO));
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Clamp, "f51e6481-2666-4a1c-99e7-c121d6f5682d"_schematyc_guid, "Clamp");
		pFunction->SetDescription("Clamp the value between min and max");
		pFunction->BindInput(1, 'val', "Value", nullptr, Vec2(ZERO));
		pFunction->BindInput(2, 'min', "Min", nullptr, Vec2(ZERO));
		pFunction->BindInput(3, 'max', "Max", nullptr, Vec2(ZERO));
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Scale, "db715617-aaf7-4f0a-a7d4-ecd0e6967c80"_schematyc_guid, "Scale");
		pFunction->SetDescription("Scale vector");
		pFunction->BindInput(1, 'val', "Input", nullptr, Vec2(ZERO)); // #SchematycTODO : Rename 'Value'!
		pFunction->BindInput(2, 'scl', "Scale", nullptr, 1.0f);
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&DotProduct, "283b624b-c536-4448-8daf-92f22d0cd02d"_schematyc_guid, "DotProduct");
		pFunction->SetDescription("Calculate dot product of two vectors");
		pFunction->BindInput(1, 'a', "A", nullptr, Vec2(ZERO));
		pFunction->BindInput(2, 'b', "B", nullptr, Vec2(ZERO));
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Angle, "20d0ed4b-70ef-4210-8349-2966d7619126"_schematyc_guid, "Angle");
		pFunction->SetDescription("Calculate angle between two vectors");
		pFunction->BindInput(1, 'a', "A", nullptr, Vec2(ZERO));
		pFunction->BindInput(2, 'b', "B", nullptr, Vec2(ZERO));
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ToString, "548ed62f-76e5-4b44-b247-751195b89605"_schematyc_guid, "ToString");
		pFunction->SetDescription("Convert Vector2 to String");
		pFunction->BindInput(1, 'val', "Input", nullptr, Vec2(ZERO)); // #SchematycTODO : Rename 'Value'!
		pFunction->BindOutput(2, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
}

} // Vector2

namespace Vector3
{

Vec3 Create(float x, float y, float z)
{
	return Vec3(x, y, z);
}

void Expand(const Vec3& input, float& x, float& y, float& z)
{
	x = input.x;
	y = input.y;
	z = input.z;
}

float Length(const Vec3& input)
{
	return input.GetLength();
}

float LengthSq(const Vec3& input)
{
	return input.GetLengthSquared();
}

float Distance(const Vec3& a, const Vec3& b)
{
	const Vec3 delta = a - b;
	return delta.GetLength();
}

float DistanceSq(const Vec3& a, const Vec3& b)
{
	const Vec3 delta = a - b;
	return delta.GetLengthSquared();
}

Vec3 Normalize(const Vec3& input)
{
	return input.GetNormalized();
}

Vec3 Add(const Vec3& a, const Vec3& b)
{
	return a + b;
}

Vec3 Subtract(const Vec3& a, const Vec3& b)
{
	return a - b;
}

Vec3 Clamp(const Vec3& value, const Vec3& min, const Vec3& max)
{
	return crymath::clamp(value, min, max);
}

Vec3 Scale(const Vec3& input, float scale)
{
	return input * scale;
}

float DotProduct(const Vec3& a, const Vec3& b)
{
	return a.Dot(b);
}

Vec3 CrossProduct(const Vec3& a, const Vec3& b)
{
	return a.Cross(b);
}

void ToString(const Vec3& input, CSharedString& output)
{
	Schematyc::ToString(output, input);
}

static void RegisterFunctions(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(GetTypeDesc<Vec3>().GetGUID());
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Create, "3496016c-8092-4a63-9198-9adce657fd1a"_schematyc_guid, "Create");
		pFunction->SetDescription("Create Vector3");
		pFunction->BindInput(1, 'x', "X");
		pFunction->BindInput(2, 'y', "Y");
		pFunction->BindInput(3, 'z', "Z");
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Expand, "e0170ced-db1d-4438-ba58-02fa2b713f59"_schematyc_guid, "Expand");
		pFunction->SetDescription("Expand Vector3");
		pFunction->BindInput(1, 'val', "Input", nullptr, Vec3(ZERO)); // #SchematycTODO : Rename 'Value'!
		pFunction->BindOutput(2, 'x', "X");
		pFunction->BindOutput(3, 'y', "Y");
		pFunction->BindOutput(4, 'z', "Z");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Length, "e8020fdb-a227-4431-b888-816ba10b6bcf"_schematyc_guid, "Length");
		pFunction->SetDescription("Calculate length of vector");
		pFunction->BindInput(1, 'val', "Input", nullptr, Vec3(ZERO)); // #SchematycTODO : Rename 'Value'!
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&LengthSq, "0705e0d3-fbe6-4e7c-afca-e3739c7df982"_schematyc_guid, "LengthSq");
		pFunction->SetDescription("Calculate squared length of vector");
		pFunction->BindInput(1, 'val', "Input", nullptr, Vec3(ZERO)); // #SchematycTODO : Rename 'Value'!
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Distance, "6138150A-CCD7-40D8-A696-0100516441D8"_schematyc_guid, "Distance");
		pFunction->SetDescription("Calculate distance between two points");
		pFunction->BindInput(1, 'a', "A", nullptr, Vec3(ZERO));
		pFunction->BindInput(2, 'b', "B", nullptr, Vec3(ZERO));
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&DistanceSq, "E2A07540-D821-4C11-A1C2-D40C1D179469"_schematyc_guid, "DistanceSq");
		pFunction->SetDescription("Calculate squared distance between two points");
		pFunction->BindInput(1, 'b', "A", nullptr, Vec3(ZERO));
		pFunction->BindInput(2, 'a', "B", nullptr, Vec3(ZERO));
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Normalize, "d953506a-6710-472d-9017-5a969f936b19"_schematyc_guid, "Normalize");
		pFunction->SetDescription("Normalize vector");
		pFunction->BindInput(1, 'val', "Input", nullptr, Vec3(ZERO)); // #SchematycTODO : Rename 'Value'!
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Add, "e50a9ee9-99dd-4011-82f7-05c67e013abb"_schematyc_guid, "Add");
		pFunction->SetDescription("Add A to B");
		pFunction->BindInput(1, 'a', "A", nullptr, Vec3(ZERO));
		pFunction->BindInput(2, 'b', "B", nullptr, Vec3(ZERO));
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Subtract, "dbafe377-fce6-430f-92e5-38b579e9833b"_schematyc_guid, "Subtract");
		pFunction->SetDescription("Subtract B from A");
		pFunction->BindInput(1, 'a', "A", nullptr, Vec3(ZERO));
		pFunction->BindInput(2, 'b', "B", nullptr, Vec3(ZERO));
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Clamp, "13ad859b-2611-4465-860a-8e6a1327ecd2"_schematyc_guid, "Clamp");
		pFunction->SetDescription("Clamp the value between min and max");
		pFunction->BindInput(1, 'val', "Value", nullptr, Vec3(ZERO));
		pFunction->BindInput(2, 'min', "Min", nullptr, Vec3(ZERO));
		pFunction->BindInput(3, 'max', "Max", nullptr, Vec3(ZERO));
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}	
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Scale, "03d7b941-57f8-4731-b518-781ea3e1dd16"_schematyc_guid, "Scale");
		pFunction->SetDescription("Scale vector");
		pFunction->BindInput(1, 'val', "Input", nullptr, Vec3(ZERO)); // #SchematycTODO : Rename 'Value'!
		pFunction->BindInput(2, 'scl', "Scale", nullptr, 1.0f);
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&DotProduct, "b2806327-f747-4dbb-905c-fadc027dbe2c"_schematyc_guid, "DotProduct");
		pFunction->SetDescription("Calculate dot product of two vectors");
		pFunction->BindInput(1, 'a', "A", nullptr, Vec3(ZERO));
		pFunction->BindInput(2, 'b', "B", nullptr, Vec3(ZERO));
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CrossProduct, "17b6dcf4-6cc9-4a04-a5a9-876857245411"_schematyc_guid, "CrossProduct");
		pFunction->SetDescription("Calculate cross product of two vectors");
		pFunction->BindInput(1, 'a', "A", nullptr, Vec3(ZERO));
		pFunction->BindInput(2, 'b', "B", nullptr, Vec3(ZERO));
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ToString, "4564e7fe-adcb-4545-8009-4acc713e305f"_schematyc_guid, "ToString");
		pFunction->SetDescription("Convert Vector3 to String");
		pFunction->BindInput(1, 'val', "Input", nullptr, Vec3(ZERO)); // #SchematycTODO : Rename 'Value'!
		pFunction->BindOutput(2, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
}

} // Vector3

namespace Rotation
{

CRotation Create(float x, float y, float z)
{
	return CRotation(Ang3(DEG2RAD(x), DEG2RAD(y), DEG2RAD(z)));
}

CRotation Look(const Vec3& forward, const Vec3& up)
{
	return CRotation(Quat(MathUtils::CreateOrthonormalTM33(forward.GetNormalizedSafe(Vec3Constants<float>::fVec3_OneY), up.GetNormalizedSafe(Vec3Constants<float>::fVec3_OneZ))));
}

void ToVector3(const CRotation& input, Vec3& right, Vec3& forward, Vec3& up)
{
	const Quat quat = input.ToQuat();
	right = quat.GetColumn0();
	forward = quat.GetColumn1();
	up = quat.GetColumn2();
}

Vec3 TransformVector3(const CRotation& rotation, const Vec3& vector)
{
	return rotation.ToQuat() * vector;
}

void ToString(const CRotation& input, CSharedString& output)
{
	Schematyc::ToString(output, input);
}

static void RegisterFunctions(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(GetTypeDesc<CRotation>().GetGUID());
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Multiply, "837695ad-ed6e-4003-b0db-f6f0127c4909"_schematyc_guid, "Multiply");
		pFunction->SetDescription("Multiply A by B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Create, "38ef172c-89cb-4310-812b-c358bccd396a"_schematyc_guid, "Create");
		pFunction->SetDescription("Create rotation");
		pFunction->BindInput(1, 'x', "X", nullptr, 0.0f);
		pFunction->BindInput(2, 'y', "Y", nullptr, 0.0f);
		pFunction->BindInput(3, 'z', "Z", nullptr, 0.0f);
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Look, "0261e9ac-0b2e-4e71-b04d-9027a9cbd3c8"_schematyc_guid, "Look");
		pFunction->SetDescription("Create rotation from two direction vectors");
		pFunction->BindInput(1, 'fwd', "Forward", nullptr, Vec3Constants<float>::fVec3_OneY);
		pFunction->BindInput(2, 'up', "Up", nullptr, Vec3Constants<float>::fVec3_OneZ);
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ToVector3, "e53f4ebd-db4a-4218-8f97-6480e6308965"_schematyc_guid, "ToVector3");
		pFunction->SetDescription("Convert rotation to three direction vectors");
		pFunction->BindInput(1, 'val', "Input"); // #SchematycTODO : Rename 'Value'!
		pFunction->BindOutput(2, 'rt', "Right");
		pFunction->BindOutput(3, 'fwd', "Forward");
		pFunction->BindOutput(4, 'up', "Up");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&TransformVector3, "1abea881-67ed-43b5-8e18-2ff79623e692"_schematyc_guid, "TransformVector3");
		pFunction->SetDescription("Use rotation to transform vector");
		pFunction->BindInput(1, 'rot', "Rotation");
		pFunction->BindInput(2, 'vec', "Vector");
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ToString, "F2CAE291-9373-482C-9554-36D9FF49BE08"_schematyc_guid, "ToString");
		pFunction->SetDescription("Convert rotation to string");
		pFunction->BindInput(1, 'val', "Input"); // #SchematycTODO : Rename 'Value'!
		pFunction->BindOutput(2, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
}

} // Rotation

namespace Transform
{

CTransform Create(const Vec3& translation, const CRotation& rotation)
{
	return CTransform(translation, rotation);
}

void Expand(const CTransform& input, Vec3& translation, CRotation& rotation)
{
	translation = input.GetTranslation();
	rotation = input.GetRotation();
}

Vec3 TransformVector3(const CTransform& rotation, const Vec3& vector)
{
	return rotation.ToQuatT() * vector;
}

static void RegisterFunctions(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(GetTypeDesc<CTransform>().GetGUID());
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Multiply, "79f1cc37-8283-49e9-93bd-8d9c9c9b3715"_schematyc_guid, "Multiply");
		pFunction->SetDescription("Multiply A by B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Create, "b901a1de-725e-4eb4-b7e8-bfdcb82a4bd5"_schematyc_guid, "Create");
		pFunction->SetDescription("Create transform");
		pFunction->BindInput(1, 'pos', "Position", nullptr, Vec3(ZERO));
		pFunction->BindInput(2, 'rot', "Rotation");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Expand, "0a70c719-9e3b-4fac-a47b-468ecabe9b56"_schematyc_guid, "Expand");
		pFunction->SetDescription("Expand transform");
		pFunction->BindInput(1, 'val', "Value");
		pFunction->BindOutput(2, 'trn', "Translation");
		pFunction->BindOutput(3, 'rot', "Rotation");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&TransformVector3, "88d9848b-c40c-4a19-ab8a-950da20267eb"_schematyc_guid, "TransformVector3");
		pFunction->SetDescription("Transform vector");
		pFunction->BindInput(1, 'trn', "Transform");
		pFunction->BindInput(2, 'vec', "Vector");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
}

} // Transform

void RegisterMathFunctions(IEnvRegistrar& registrar)
{
	Float::RegisterFunctions(registrar);
	Vector2::RegisterFunctions(registrar);
	Vector3::RegisterFunctions(registrar);
	Rotation::RegisterFunctions(registrar);
	Transform::RegisterFunctions(registrar);
}

} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::RegisterMathFunctions)
