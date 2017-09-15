// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "NavigationSystem/NavigationSystem.h"

bool Serialize(Serialization::IArchive& archive, NavigationAgentTypeID& value, const char* szName, const char* szLabel);

inline void NavigationAgentTypeIDToString(Schematyc::IString& output, const NavigationAgentTypeID& input)
{
	output.assign(gAIEnv.pNavigationSystem->GetAgentTypeName(input));
}

inline void ReflectType(Schematyc::CTypeDesc<NavigationAgentTypeID>& desc)
{
	desc.SetGUID("b50d458b-faeb-4d12-98fd-3d2543d4e68d"_cry_guid);
	desc.SetLabel("NavAgentType");
	desc.SetDescription("NavigationAgentTypeId");
	desc.SetDefaultValue(NavigationAgentTypeID());
	desc.SetToStringOperator<&NavigationAgentTypeIDToString>();
}

namespace NavigationSystemSchematyc
{
	void Register(Schematyc::IEnvRegistrar& registrar, Schematyc::CEnvRegistrationScope& parentScope);

} //NavigationSystemSchematyc