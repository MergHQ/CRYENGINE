// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/FundamentalTypes.h"
#include <Schematyc/Env/EnvElementBase.h>
#include <Schematyc/Env/Elements/IEnvModule.h>

#define SCHEMATYC_MAKE_ENV_MODULE(guid, name) Schematyc::EnvModule::MakeShared(guid, name, SCHEMATYC_SOURCE_FILE_INFO)

namespace Schematyc
{

class CEnvModule : public CEnvElementBase<IEnvModule>
{
public:

	inline CEnvModule(const SGUID& guid, const char* szName, const SSourceFileInfo& sourceFileInfo)
		: CEnvElementBase(guid, szName, sourceFileInfo)
	{}

	// IEnvElement

	virtual bool IsValidScope(IEnvElement& scope) const override
	{
		switch (scope.GetType())
		{
		case EEnvElementType::Root:
		case EEnvElementType::Module:
			{
				return true;
			}
		default:
			{
				return false;
			}
		}
	}

	// ~IEnvElement
};

namespace EnvModule
{

inline std::shared_ptr<CEnvModule> MakeShared(const SGUID& guid, const char* szName, const SSourceFileInfo& sourceFileInfo)
{
	return std::make_shared<CEnvModule>(guid, szName, sourceFileInfo);
}

} // EnvModule
} // Schematyc
