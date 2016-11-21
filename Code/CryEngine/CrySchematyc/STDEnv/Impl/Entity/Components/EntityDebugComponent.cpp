// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityDebugComponent.h"

#include <CryRenderer/IRenderAuxGeom.h>
#include <Schematyc/Entity/EntityClasses.h>
#include <Schematyc/Entity/EntityUtils.h>

#include "AutoRegister.h"
#include "STDModules.h"

namespace Schematyc
{
bool CEntityDebugComponent::Init()
{
	return true;
}

void CEntityDebugComponent::Run(ESimulationMode simulationMode) {}

void  CEntityDebugComponent::Shutdown() {}

void CEntityDebugComponent::DrawText(const CSharedString& text, const Vec2& pos, const ColorF& color)
{
	IRenderAuxText::Draw2dLabel(pos.x, pos.y, 2.0f, color, false, "%s", text.c_str());
}

SGUID CEntityDebugComponent::ReflectSchematycType(CTypeInfo<CEntityDebugComponent>& typeInfo)
{
	return "082bb332-101f-48a7-8c78-b9f19f9e6ef2"_schematyc_guid;
}

void CEntityDebugComponent::Register(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(g_entityClassGUID);
	{
		auto pComponent = SCHEMATYC_MAKE_ENV_COMPONENT(CEntityDebugComponent, "Debug");
		pComponent->SetAuthor(g_szCrytek);
		pComponent->SetDescription("Entity debug component");
		pComponent->SetIcon("icons:schematyc/debug.ico");
		pComponent->SetFlags(EEnvComponentFlags::Singleton);
		scope.Register(pComponent);

		CEnvRegistrationScope componentScope = registrar.Scope(pComponent->GetGUID());
		// Functions
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityDebugComponent::DrawText, "43e5376b-c860-4054-94a3-f0b47821e367"_schematyc_guid, "DrawText");
			pComponent->SetAuthor(g_szCrytek);
			pComponent->SetDescription("Draw debug text");
			pFunction->BindInput(1, 'txt', "Text");
			pFunction->BindInput(2, 'pos', "Position", "", Vec2(ZERO));
			pFunction->BindInput(3, 'col', "Color", "", Col_White);
			componentScope.Register(pFunction);
		}
	}
}
} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::CEntityDebugComponent::Register)
