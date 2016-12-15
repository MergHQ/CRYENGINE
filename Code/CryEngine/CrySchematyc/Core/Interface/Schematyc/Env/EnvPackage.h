// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Env/IEnvPackage.h"

#define SCHEMATYC_MAKE_ENV_PACKAGE(guid, name, callback) stl::make_unique<Schematyc::CEnvPackage>(guid, name, callback)

namespace Schematyc
{
typedef std::unique_ptr<IEnvPackage> IEnvPackagePtr;

class CEnvPackage : public IEnvPackage
{
public:

	inline CEnvPackage(const SGUID& guid, const char* szName, const EnvPackageCallback& callback)
		: m_guid(guid)
		, m_name(szName)
		, m_callback(callback)
	{}

	// IEnvPackage

	virtual SGUID GetGUID() const override
	{
		return m_guid;
	}

	virtual const char* GetName() const override
	{
		return m_name.c_str();
	}

	virtual EnvPackageCallback GetCallback() const override
	{
		return m_callback;
	}

	// ~IEnvPackage

private:

	SGUID              m_guid;
	string             m_name;
	EnvPackageCallback m_callback;
};
} // Schematyc
