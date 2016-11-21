// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryString/CryStringUtils.h>

namespace Schematyc
{
inline void Ang3ToString(IString& output, const Ang3& input)
{
	char temp[154] = "";
	cry_sprintf(temp, sizeof(temp), "%.8f, %.8f, %.8f", input.x, input.y, input.z);
	output.assign(temp);
}

inline void QuatToString(IString& output, const Quat& input)
{
	char temp[208] = "";
	cry_sprintf(temp, sizeof(temp), "%.8f, %.8f, %.8f", input.v.x, input.v.y, input.v.z, input.w);
	output.assign(temp);
}
} // Schematyc
