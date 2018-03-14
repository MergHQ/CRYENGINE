// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "EntityComponentsVector.h"
#include "Entity.h"

void SEntityComponentRecord::Shutdown()
{
	static_cast<CEntity*>(pComponent->GetEntity())->ShutDownComponent(*this);
}
