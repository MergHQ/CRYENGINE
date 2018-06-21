// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <CryReflection/Framework.h>

void ReflectVec2i(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("Vec2i");
	typeDesc.SetDescription("2D vector with 32-bit signed integral types.");
}
CRY_REFLECT_TYPE(Vec2i, "{2D5EFC8D-99F9-4B77-8E31-F23BC81BB1DD}"_cry_guid, ReflectVec2i);

void ReflectVec2(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("Vec2");
	typeDesc.SetDescription("2D vector with 32-bit floating point types.");
}
CRY_REFLECT_TYPE(Vec2, "{2FDF32B7-FE95-40B9-A3D8-77B38AD8E28A}"_cry_guid, ReflectVec2);

void ReflectVec3i(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("Vec3i");
	typeDesc.SetDescription("3D vector with 32-bit signed integral types.");
}
CRY_REFLECT_TYPE(Vec3i, "{3D6C1A9D-FC0F-4887-8EBD-04262DBB931C}"_cry_guid, ReflectVec3i);

void ReflectVec3(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("Vec3");
	typeDesc.SetDescription("3D vector with 32-bit floating point types.");
}
CRY_REFLECT_TYPE(Vec3, "{3FC8C371-4B5D-41AB-B3BE-86DD34B29809}"_cry_guid, ReflectVec3);

void ReflectAngles3(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("Angle3");
	typeDesc.SetDescription("Angle in 3 axes.");
}
CRY_REFLECT_TYPE(CryTransform::CAngles3, "{A33FED5A-3565-4D92-A9A1-A7178615A230}"_cry_guid, ReflectAngles3);

void ReflectAngle(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("Angle");
	typeDesc.SetDescription("Used to simplify conversion between radians and degrees.");
}
CRY_REFLECT_TYPE(CryTransform::CAngle, "{A14C17DF-5717-4499-83A2-34FF7B75A195}"_cry_guid, ReflectAngle);

void ReflectMatrix33(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("Matrix33");
	typeDesc.SetDescription("Represents a 3x3 rotational matrix.");
}
CRY_REFLECT_TYPE(Matrix33, "{3373C591-A3D3-4FAE-AC6D-049A53D76F92}"_cry_guid, ReflectMatrix33);

void ReflectMatrix34(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("Matrix34");
	typeDesc.SetDescription("Represents a 3x4 matrix.");
}
CRY_REFLECT_TYPE(Matrix34, "{341ACD07-FD4A-4F02-8EAB-571138678817}"_cry_guid, ReflectMatrix34);

void ReflectMatrix44(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("Matrix44");
	typeDesc.SetDescription("Represents a 4x4 matrix.");
}
CRY_REFLECT_TYPE(Matrix44, "{449B18C3-52AE-48E0-A263-88C3F9FFC2A2}"_cry_guid, ReflectMatrix44);

void ReflectQuat(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("Quat");
	typeDesc.SetDescription("Quaternion implementation.");
}
CRY_REFLECT_TYPE(Quat, "{88FF5F81-844F-4248-A024-B9CEF7CA259C}"_cry_guid, ReflectQuat);
