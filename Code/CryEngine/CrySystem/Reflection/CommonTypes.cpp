// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <CryReflection/Framework.h>

void ReflectString(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("string");
	typeDesc.SetDescription("Reference-tracked character string implementation.");
}
CRY_REFLECT_TYPE(string, "{520DBDF6-8211-43F7-AA43-B464083D8188}"_cry_guid, ReflectString);

void ReflectWString(Cry::Reflection::ITypeDesc& typeDesc)
{
	typeDesc.SetLabel("wstring");
	typeDesc.SetDescription("Reference-tracked wide character string implementation.");
}
CRY_REFLECT_TYPE(wstring, "{52899742-E340-43C9-812B-E9EA3FD39132}"_cry_guid, ReflectWString);
