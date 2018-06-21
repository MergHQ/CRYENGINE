// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <CryReflection/Framework.h>

void ReflectBool(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("bool");
	typeDesc.SetDescription("A 1-byte integral type that can have one of the two values true or false.");
}
CRY_REFLECT_TYPE(bool, "{BB67FE0B-744E-4253-AD72-03D80CD1734B}"_cry_guid, ReflectBool);

void ReflectChar(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("char");
	typeDesc.SetDescription("A 1-byte integral type that usually contains members of the basic execution character set.");
}
CRY_REFLECT_TYPE(char, "{CC1A3C72-4819-46F6-B8F6-992D9F45CCFB}"_cry_guid, ReflectChar);

void ReflectFloat(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("float");
	typeDesc.SetDescription("A 4-byte floating point type.");
}
CRY_REFLECT_TYPE(float, "{FF1355EC-96DA-4D56-87BB-D36A2A2CDAD3}"_cry_guid, ReflectFloat);

void ReflectUInt64(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("uint64");
	typeDesc.SetDescription("A 8-byte unsigned integral type.");
}
CRY_REFLECT_TYPE(uint64, "{64BE4B59-A9CD-46B3-8944-77349FD2F456}"_cry_guid, ReflectUInt64);

void ReflectUInt32(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("uint32");
	typeDesc.SetDescription("A 4-byte unsigned integral type.");
}
CRY_REFLECT_TYPE(uint32, "{3202DE0D-FD01-4B75-B536-F27B4C305E14}"_cry_guid, ReflectUInt32);

void ReflectUInt16(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("uint16");
	typeDesc.SetDescription("A 2-byte unsigned integral type.");
}
CRY_REFLECT_TYPE(uint16, "{163AA70B-7DEA-435A-B726-BFDAFB6AA2F7}"_cry_guid, ReflectUInt16);

void ReflectUInt8(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("uint8");
	typeDesc.SetDescription("A 1-byte unsigned integral type.");
}
CRY_REFLECT_TYPE(uint8, "{82EA5C18-75D3-4333-8E95-B6BE1A79EFB3}"_cry_guid, ReflectUInt8);

void ReflectInt64(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("int64");
	typeDesc.SetDescription("A 8-byte signed integral type.");
}
CRY_REFLECT_TYPE(int64, "{64E9AB2C-E182-45B2-822C-3E2BD70B96FE}"_cry_guid, ReflectInt64);

void ReflectInt32(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("int32");
	typeDesc.SetDescription("A 4-byte signed integral type.");
}
CRY_REFLECT_TYPE(int32, "{32AA5FA9-5F45-42D7-9F9A-F919567BE0C7}"_cry_guid, ReflectInt32);

void ReflectInt16(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("int16");
	typeDesc.SetDescription("A 2-byte signed integral type.");
}
CRY_REFLECT_TYPE(int16, "{16EF4C9A-9BAB-4B5D-94B1-20D8C0DA75DA}"_cry_guid, ReflectInt16);

void ReflectInt8(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("int8");
	typeDesc.SetDescription("A 1-byte unsigned integral type.");
}
CRY_REFLECT_TYPE(int8, "{8BD5825E-1878-4D11-9AE9-911109CFDCAE}"_cry_guid, ReflectInt8);
