// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "AutoRegister.h"
#include "STDModules.h"

namespace Schematyc
{
namespace Log
{
void Comment(const CSharedString& message, const SLogStreamName& streamName)
{
	SCHEMATYC_COMMENT(gEnv->pSchematyc->GetLog().GetStreamId(streamName.value.c_str()), message.c_str());
}

void Warning(const CSharedString& message, const SLogStreamName& streamName)
{
	SCHEMATYC_WARNING(gEnv->pSchematyc->GetLog().GetStreamId(streamName.value.c_str()), message.c_str());
}

void Error(const CSharedString& message, const SLogStreamName& streamName)
{
	SCHEMATYC_ERROR(gEnv->pSchematyc->GetLog().GetStreamId(streamName.value.c_str()), message.c_str());
}

static void RegisterFunctions(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(g_logModuleGUID);
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Comment, "71c2cf50-8bbb-4d35-ba71-f6ffa895141d"_schematyc_guid, "Comment");
		pFunction->SetAuthor(g_szCrytek);
		pFunction->SetDescription("Send comment to log");
		pFunction->BindInput(1, 'msg', "Message");
		pFunction->BindInput(2, 'str', "Stream");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Warning, "8f9dd16b-b1e7-40fd-af24-93b2eb887a8a"_schematyc_guid, "Warning");
		pFunction->SetAuthor(g_szCrytek);
		pFunction->SetDescription("Send warning to log");
		pFunction->BindInput(1, 'msg', "Message");
		pFunction->BindInput(2, 'str', "Stream");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Error, "feb709b5-3ebe-4186-81e9-ec8d49f30397"_schematyc_guid, "Error");
		pFunction->SetAuthor(g_szCrytek);
		pFunction->SetDescription("Send error to log");
		pFunction->BindInput(1, 'msg', "Message");
		pFunction->BindInput(2, 'str', "Stream");
		scope.Register(pFunction);
	}
}
} // Log
} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::Log::RegisterFunctions)
