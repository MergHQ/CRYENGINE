// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef SCHEMATYC_REFLECTION_HEADER
	#error This file should only be included from Reflection.h!
#endif

namespace Schematyc
{
template<typename TYPE> bool ToString(IString& output, const TYPE& input)
{
	const CCommonTypeInfo& typeInfo = GetTypeInfo<TYPE>();
	if (typeInfo.GetMethods().toString)
	{
		typeInfo.GetMethods().toString(output, &input);
		return true;
	}
	return false;
}
} // Schematyc
