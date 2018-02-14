// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryEntitySystem/IEntityComponent.h>

struct IEntityObserverComponent : public IEntityComponent
{
	static void ReflectType(Schematyc::CTypeDesc<IEntityObserverComponent>& desc)
	{
		desc.SetGUID("EC4E135E-BB29-48DB-B143-CB9206F4B2E6"_cry_guid);
	}
};