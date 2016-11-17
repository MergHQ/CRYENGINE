// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Cry3DEngine/I3DEngine.h> // Must be included before CREOcclusionQuery.h.
#include <CryEntitySystem/IEntityRenderState.h>

namespace Schematyc
{
struct SEntityNotHidden
{
	inline bool operator()(IEntity* pEntity) const
	{
		return pEntity && !pEntity->IsHidden();
	}
};

struct SEntityVisible
{
	inline bool operator()(IEntity* pEntity) const
	{
		if (pEntity)
		{
			IEntityRenderProxy* pRenderProxy = reinterpret_cast<IEntityRenderProxy*>(pEntity->GetProxy(ENTITY_PROXY_RENDER));
			if (pRenderProxy)
			{
				IRenderNode* pRenderNode = pRenderProxy->GetRenderNode();
				if (pRenderNode)
				{
					return (gEnv->pRenderer->GetFrameID() - pRenderNode->GetDrawFrame()) < 5;
				}
			}
		}
		return false;
	}
};

typedef CConfigurableUpdateFilter<IEntity*, NTypelist::CConstruct<SEntityNotHidden>::TType>                 EntityNotHiddenUpdateFilter;
typedef CConfigurableUpdateFilter<IEntity*, NTypelist::CConstruct<SEntityNotHidden, SEntityVisible>::TType> EntityVisibleUpdateFilter;
} // Schematyc
