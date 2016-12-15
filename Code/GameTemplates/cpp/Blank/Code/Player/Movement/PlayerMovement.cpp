#include "StdAfx.h"
#include "PlayerMovement.h"

#include "Player/Player.h"
#include "Player/Input/PlayerInput.h"

#include "GamePlugin.h"

class CPlayerMovementRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		CGamePlugin::RegisterEntityComponent<CPlayerMovement>("PlayerMovement");

		REGISTER_CVAR2("pl_movementSpeed", &m_movementSpeed, 20.0f, VF_NULL, "Player movement speed.");
	}

	virtual void Unregister() override
	{
		IConsole* pConsole = gEnv->pConsole;
		if (pConsole)
		{
			pConsole->UnregisterVariable("pl_movementSpeed");
		}
	}

public:
	CPlayerMovementRegistrator() {}

	~CPlayerMovementRegistrator()
	{
		gEnv->pConsole->UnregisterVariable("pl_movementSpeed", true);
	}

	float m_movementSpeed;
};

CPlayerMovementRegistrator g_playerMovementRegistrator;

CPlayerMovement::CPlayerMovement()
{
}

void CPlayerMovement::PostInit(IGameObject *pGameObject)
{
	m_pPlayer = static_cast<CPlayer *>(pGameObject->QueryExtension("Player"));

	// Make sure that this extension is updated regularly via the Update function below
	pGameObject->EnableUpdateSlot(this, 0);
}

void CPlayerMovement::Update(SEntityUpdateContext &ctx, int updateSlot)
{
	Matrix34 playerTransform = GetEntity()->GetWorldTM();

	// Start with calculating movement direction
	Vec3 moveDirection = m_pPlayer->GetInput()->GetMovementDirection();
	moveDirection *= g_playerMovementRegistrator.m_movementSpeed;

	// Add move direction to player position
	playerTransform.AddTranslation(GetEntity()->GetWorldRotation() * moveDirection * ctx.fFrameTime);

	// Update view rotation based on input
	auto mouseDeltaRotation = m_pPlayer->GetInput()->GetAndResetMouseDeltaRotation();
	if (!mouseDeltaRotation.IsZero())
	{
		Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(playerTransform));

		ypr.x += mouseDeltaRotation.x * ctx.fFrameTime * 0.05f;
		ypr.y += mouseDeltaRotation.y * ctx.fFrameTime * 0.05f;
		ypr.y = clamp_tpl(ypr.y, -(float)g_PI * 0.5f, (float)g_PI * 0.5f);
		ypr.z = 0;

		playerTransform.SetRotation33(CCamera::CreateOrientationYPR(ypr));
	}

	// Now set the new player transform on the entity
	GetEntity()->SetWorldTM(playerTransform);
}