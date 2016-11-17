// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CoreEnvSignals.h"

#include <Schematyc/Env/IEnvRegistrar.h>
#include <Schematyc/Env/Elements/EnvSignal.h>

#include "CoreEnvModules.h"

namespace Schematyc
{
const SGUID g_startSignalGUID = "a9279137-7c66-491d-b9a0-8752c24c8979"_schematyc_guid;
const SGUID g_stopSignalGUID = "52ad5add-7781-429b-b4a9-0cb6c905e353"_schematyc_guid;
const SGUID g_updateSignalGUID = "b2561caa-0753-458b-a91f-e8e38b0f0cdf"_schematyc_guid;

void RegisterCoreEnvSignals(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(g_coreModuleGUID);
	{
		auto pSignal = SCHEMATYC_MAKE_ENV_SIGNAL(g_startSignalGUID, "Start");
		pSignal->SetAuthor("Paul Slinger");
		pSignal->SetDescription("Sent when object starts running.");
		scope.Register(pSignal);
	}
	{
		auto pSignal = SCHEMATYC_MAKE_ENV_SIGNAL(g_stopSignalGUID, "Stop");
		pSignal->SetAuthor("Paul Slinger");
		pSignal->SetDescription("Sent when object stops running.");
		scope.Register(pSignal);
	}
	{
		auto pSignal = SCHEMATYC_MAKE_ENV_SIGNAL(g_updateSignalGUID, "Update");
		pSignal->SetAuthor("Paul Slinger");
		pSignal->SetDescription("Sent when object updates.");
		pSignal->AddInput('time', "FrameTime", "Time(s) since last update", 0.0f);
		scope.Register(pSignal);
	}
}
} // Schematyc
