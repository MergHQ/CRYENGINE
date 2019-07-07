// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICmdLine.h>

class CCmdLineArg :	public ICmdLineArg
{
public:
	CCmdLineArg(const char* name, const char* value, ECmdLineArgType type);

	const char*           GetName() const override;
	const char*           GetValue() const override;
	const ECmdLineArgType GetType() const override;
	const float           GetFValue() const override;
	const int             GetIValue() const override;

private:
	string          m_name;
	string          m_value;
	ECmdLineArgType m_type;
};
