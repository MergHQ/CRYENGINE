// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.


#include "StdAfx.h"
#include "MovementExtension.h"


CMovementExtension::CMovementExtension()
	: m_movementSpeed(0.0f)
	, m_boostMultiplier(0.0f)
{
}

CMovementExtension::~CMovementExtension()
{
}

void CMovementExtension::Release()
{
	gEnv->pConsole->UnregisterVariable("gamezero_pl_movementSpeed", true);
	gEnv->pConsole->UnregisterVariable("gamezero_pl_boostMultiplier", true);

	ISimpleExtension::Release();
}

void CMovementExtension::PostInit(IGameObject* pGameObject)
{
	pGameObject->EnablePostUpdates(this);

	REGISTER_CVAR2("gamezero_pl_movementSpeed", &m_movementSpeed, 20.0f, VF_NULL, "Player movement speed.");
	REGISTER_CVAR2("gamezero_pl_boostMultiplier", &m_boostMultiplier, 2.0f, VF_NULL, "Player boost multiplier.");
}

void CMovementExtension::PostUpdate(float frameTime)
{
	SmartScriptTable inputParams(gEnv->pScriptSystem);
	if (GetGameObject()->GetExtensionParams("InputExtension", inputParams))
	{
		Ang3 rotation(ZERO);
		inputParams->GetValue("rotation", rotation);
	
		Quat viewRotation(rotation);		
		GetEntity()->SetRotation(viewRotation.GetNormalized());

		Vec3 vDeltaMovement(ZERO);
		inputParams->GetValue("deltaMovement", vDeltaMovement);

		bool bBoost = false;
		inputParams->GetValue("boost", bBoost);

		const float boostMultiplier = bBoost ? m_boostMultiplier : 1.0f;

		GetEntity()->SetPos(GetEntity()->GetWorldPos() + viewRotation * (vDeltaMovement * frameTime * boostMultiplier * m_movementSpeed));
	}
}
