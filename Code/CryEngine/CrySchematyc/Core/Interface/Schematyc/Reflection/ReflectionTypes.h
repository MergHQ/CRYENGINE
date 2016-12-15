// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef SCHEMATYC_REFLECTION_HEADER
#error This file should only be included from Reflection.h!
#endif

namespace Schematyc
{
// Reflect 'bool' type.
////////////////////////////////////////////////////////////////////////////////////////////////////

inline void BoolToString(IString& output, const bool& input)
{
	output.assign(input ? "1" : "0");
}

inline SGUID ReflectSchematycType(CTypeInfo<bool>& typeInfo)
{
	typeInfo.SetToStringMethod<&BoolToString>();
	return "03211ee1-b8d3-42a5-bfdc-296fc535fe43"_schematyc_guid;
}

// Reflect 'int32' type.
////////////////////////////////////////////////////////////////////////////////////////////////////

inline void Int32ToString(IString& output, const int32& input)
{
	char temp[16] = "";
	itoa(input, temp, 10);
	output.assign(temp);
}

inline SGUID ReflectSchematycType(CTypeInfo<int32>& typeInfo)
{
	typeInfo.SetToStringMethod<&Int32ToString>();
	return "8bd755fd-ed92-42e8-a488-79e7f1051b1a"_schematyc_guid;
}

// Reflect 'uint32' type.
////////////////////////////////////////////////////////////////////////////////////////////////////

inline void UInt32ToString(IString& output, const uint32& input)
{
	char temp[16] = "";
	ltoa(input, temp, 10);
	output.assign(temp);
}

inline SGUID ReflectSchematycType(CTypeInfo<uint32>& typeInfo)
{
	typeInfo.SetToStringMethod<&UInt32ToString>();
	return "db4b8137-5150-4246-b193-d8d509bec2d4"_schematyc_guid;
}

// Reflect 'float' type.
////////////////////////////////////////////////////////////////////////////////////////////////////

inline void FloatToString(IString& output, const float& input)
{
	output.Format("%.8f", input);
}

inline SGUID ReflectSchematycType(CTypeInfo<float>& typeInfo) // #SchematycTODO : Move to STDEnv/MathTypes.h?
{
	typeInfo.SetToStringMethod<&FloatToString>();
	return "03d99d5a-cf2c-4f8a-8489-7da5b515c202"_schematyc_guid;
}

// Reflect 'Vec2' type.
////////////////////////////////////////////////////////////////////////////////////////////////////

inline void Vec2ToString(IString& output, const Vec2& input)
{
	output.Format("%.8f, %.8f", input.x, input.y);
}

inline SGUID ReflectSchematycType(CTypeInfo<Vec2>& typeInfo) // #SchematycTODO : Move to STDEnv/MathTypes.h?
{
	typeInfo.SetToStringMethod<&Vec2ToString>();
	typeInfo.DeclareSerializeable();
	return "6c607bf5-76d7-45d0-9b34-9d81a13c3c89"_schematyc_guid;
}

// Reflect 'Vec3' type.
////////////////////////////////////////////////////////////////////////////////////////////////////

inline void Vec3ToString(IString& output, const Vec3& input)
{
	output.Format("%.8f, %.8f, %.8f", input.x, input.y, input.z);
}

inline SGUID ReflectSchematycType(CTypeInfo<Vec3>& typeInfo) // #SchematycTODO : Move to STDEnv/MathTypes.h?
{
	typeInfo.SetToStringMethod<&Vec3ToString>();
	typeInfo.DeclareSerializeable();
	return "e01bd066-2a42-493d-bc43-436b233833d8"_schematyc_guid;
}

// Reflect 'ColorF' type.
////////////////////////////////////////////////////////////////////////////////////////////////////

inline void ColorFToString(IString& output, const ColorF& input)
{
	output.Format("%.8f, %.8f, %.8f, %.8f", input.r, input.g, input.b, input.a);
}

inline SGUID ReflectSchematycType(CTypeInfo<ColorF>& typeInfo) // #SchematycTODO : Move to STDEnv/BasicTypes.h?
{
	typeInfo.SetToStringMethod<&ColorFToString>();
	typeInfo.DeclareSerializeable();
	return "ec4b79e2-f585-4468-8b91-db01e4f84bfa"_schematyc_guid;
}

// Reflect 'SGUID' type.
////////////////////////////////////////////////////////////////////////////////////////////////////

inline SGUID ReflectSchematycType(CTypeInfo<SGUID>& typeInfo)
{
	typeInfo.SetToStringMethod<&GUID::ToString>();
	return "816b083f-b77d-4133-ae84-8d24319b609e"_schematyc_guid;
}

// Reflect 'ObjectId' type.
////////////////////////////////////////////////////////////////////////////////////////////////////

inline void ObjectIdToString(IString& output, const ObjectId& input)
{
	output.Format("%d", static_cast<uint32>(input));
}

inline SGUID ReflectSchematycType(CTypeInfo<ObjectId>& typeInfo)
{
	typeInfo.SetToStringMethod<&ObjectIdToString>();
	typeInfo.DeclareSerializeable();
	return "95b8918e-9e65-4b6c-9c48-8899754f9d3c"_schematyc_guid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline SGUID ReflectSchematycType(CTypeInfo<EComparisonMode>& typeInfo) // #SchematycTODO : This should be declared in the same header as EComparisonMode!
{
	typeInfo.AddConstant(EComparisonMode::Equal, "Equal", "Equal");
	typeInfo.AddConstant(Schematyc::EComparisonMode::NotEqual, "NotEqual", "Not equal");
	typeInfo.AddConstant(Schematyc::EComparisonMode::LessThan, "LessThan", "Less than");
	typeInfo.AddConstant(Schematyc::EComparisonMode::LessThanOrEqual, "LessThanOrEqual", "Less than or equal");
	typeInfo.AddConstant(Schematyc::EComparisonMode::GreaterThan, "GreaterThan", "Greater than");
	typeInfo.AddConstant(Schematyc::EComparisonMode::GreaterThanOrEqual, "GreaterThanOrEqual", "Greater than or equal");
	return "bfb6294e-cf7b-43ef-ae2e-55e6d4e7bd43"_schematyc_guid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
} // Schematyc
