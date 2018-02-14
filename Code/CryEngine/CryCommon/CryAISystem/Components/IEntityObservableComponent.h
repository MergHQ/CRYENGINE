// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryEntitySystem/IEntityComponent.h>

struct IEntityObservableComponent : public IEntityComponent
{
	static void ReflectType(Schematyc::CTypeDesc<IEntityObservableComponent>& desc)
	{
		desc.SetGUID("A2406F9B-F650-4207-BA05-EC0649D1F166"_cry_guid);
	}
};