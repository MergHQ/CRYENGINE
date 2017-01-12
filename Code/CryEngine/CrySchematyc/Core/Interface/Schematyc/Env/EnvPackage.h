// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Env/IEnvPackage.h"

#define SCHEMATYC_MAKE_ENV_PACKAGE(guid, name, author, description, callback) stl::make_unique<Schematyc::CEnvPackage>(guid, name, author, description, callback)

namespace Schematyc
{

typedef std::unique_ptr<IEnvPackage> IEnvPackagePtr;

class CEnvPackage : public IEnvPackage
{
public:

	inline CEnvPackage(const SGUID& guid, const char* szName, const char* szAuthor, const char* szDescription, const EnvPackageCallback& callback)
		: m_guid(guid)
		, m_name(szName)
		, m_author(szAuthor)
		, m_description(szDescription)
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

	virtual const char* GetAuthor() const override
	{
		return m_author.c_str();
	}

	virtual const char* GetDescription() const override
	{
		return m_description.c_str();
	}

	virtual EnvPackageCallback GetCallback() const override
	{
		return m_callback;
	}

	// ~IEnvPackage

private:

	SGUID              m_guid;
	string             m_name;
	string             m_author;
	string             m_description;
	EnvPackageCallback m_callback;
};

} // Schematyc
