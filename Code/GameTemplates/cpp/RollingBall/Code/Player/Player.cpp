#include "StdAfx.h"
#include "Player.h"

#include "Movement/PlayerMovement.h"
#include "Input/PlayerInput.h"
#include "View/PlayerView.h"

#include "GamePlugin.h"
#include "Game/GameRules.h"

#include "Entities/Gameplay/SpawnPoint.h"

CRYREGISTER_CLASS(CPlayer);

class CPlayerRegistrator
	: public IEntityRegistrator
	, public CPlayer::SExternalCVars
{
	virtual void Register() override
	{
		RegisterEntityWithDefaultComponent<CPlayer>("Player");
		RegisterCVars();
	}

	virtual void Unregister() override
	{
		UnregisterCVars();
	}

	void RegisterCVars()
	{
		REGISTER_CVAR2("pl_mass", &m_mass, 90.f, VF_CHEAT, "Mass of the player entity in kg");
		REGISTER_CVAR2("pl_moveImpulseStrength", &m_moveImpulseStrength, 800.f, VF_CHEAT, "Strength of the per-frame impulse when holding inputs");

		REGISTER_CVAR2("pl_rotationSpeedYaw", &m_rotationSpeedYaw, 0.05f, VF_CHEAT, "Speed at which the player rotates entity yaw");
		REGISTER_CVAR2("pl_rotationSpeedPitch", &m_rotationSpeedPitch, 0.05f, VF_CHEAT, "Speed at which the player rotates entity pitch");

		REGISTER_CVAR2("pl_rotationLimitsMinPitch", &m_rotationLimitsMinPitch, -0.84f, VF_CHEAT, "Minimum entity pitch limit");
		REGISTER_CVAR2("pl_rotationLimitsMaxPitch", &m_rotationLimitsMaxPitch, 1.5f, VF_CHEAT, "Maximum entity pitch limit");

		REGISTER_CVAR2("pl_viewDistance", &m_viewDistance, 10.f, VF_CHEAT, "Determines the distance between player and camera");
		
		m_pGeometry = REGISTER_STRING("pl_geometry", "Objects/Default/primitive_sphere.cgf", VF_CHEAT, "Sets the third person geometry to load");
	}

	void UnregisterCVars()
	{
		IConsole* pConsole = gEnv->pConsole;
		if (pConsole)
		{
			pConsole->UnregisterVariable("pl_mass");
			pConsole->UnregisterVariable("pl_moveImpulseStrength");
			pConsole->UnregisterVariable("pl_rotationSpeedYaw");
			pConsole->UnregisterVariable("pl_rotationSpeedPitch");
			pConsole->UnregisterVariable("pl_rotationLimitsMinPitch");
			pConsole->UnregisterVariable("pl_rotationLimitsMaxPitch");
			pConsole->UnregisterVariable("pl_viewDistance");
			pConsole->UnregisterVariable("pl_geometry");
		}
	}
};

CPlayerRegistrator g_playerRegistrator;

CPlayer::CPlayer()
	: m_pInput(nullptr)
	, m_pMovement(nullptr)
	, m_pView(nullptr)
{
}

CPlayer::~CPlayer()
{
}

const CPlayer::SExternalCVars &CPlayer::GetCVars() const
{
	return g_playerRegistrator;
}

void CPlayer::Initialize()
{
	m_pMovement = GetEntity()->CreateComponent<CPlayerMovement>();
	m_pInput = GetEntity()->CreateComponent<CPlayerInput>();
	m_pView = GetEntity()->CreateComponent<CPlayerView>();
	
	// Set the player geometry, this also triggers physics proxy creation.
	// Has to be done before binding to network to replicate physics.
	SetPlayerModel();
}

uint64 CPlayer::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_RESET)
		| BIT64(ENTITY_EVENT_NET_BECOME_LOCAL_PLAYER)
	;
}

void CPlayer::ProcessEvent(SEntityEvent& event)
{
	switch (event.event)
	{
		case ENTITY_EVENT_RESET:
		{
			if (event.nParam[0] == 1)
			{
				// Make sure to revive player when respawning in Editor
				Respawn();
			}
		}
		break;
		case ENTITY_EVENT_NET_BECOME_LOCAL_PLAYER:
		{
			m_pInput->OnPlayerRespawn();
			GetEntity()->Activate(true);
		}
		break;
	}
}

void CPlayer::Respawn()
{
	GetEntity()->Activate(true);

	// Find a spawn point and move the entity there
	SelectSpawnPoint();

	// Unhide the entity in case hidden by the Editor
	GetEntity()->Hide(false);

	// Notify input that the player respawned
	m_pInput->OnPlayerRespawn();
}

void CPlayer::DisplayText(const Vec3 &pos, const char *text)
{
	IPersistantDebug *dbg = gEnv->pGameFramework->GetIPersistantDebug();
	dbg->Begin("PlayerText", false);
	dbg->AddText3D(pos + Vec3(0, 0, 1.4), 1.5f, Vec3(0, 1, 0), 5.0, text);
}

void CPlayer::SelectSpawnPoint()
{
	// We only handle default spawning below for the Launcher
	// Editor has special logic in CEditorGame
	if (gEnv->IsEditor())
		return;

	// Spawn at first default spawner
	auto *pEntityIterator = gEnv->pEntitySystem->GetEntityIterator();
	pEntityIterator->MoveFirst();

	auto *pSpawnerClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("SpawnPoint");
	while (!pEntityIterator->IsEnd())
	{
		IEntity *pEntity = pEntityIterator->Next();

		if (pEntity->GetClass() != pSpawnerClass)
			continue;

		if (auto* pSpawner = pEntity->GetComponent<CSpawnPoint>())
		{
			pSpawner->SpawnEntity(*GetEntity());
		}
		break;
	}
}

void CPlayer::SetPlayerModel()
{
	// Load the third person model
	GetEntity()->LoadGeometry(eGeometry_ThirdPerson, GetCVars().m_pGeometry->GetString());
	
	// Override material so that we can see the ball rolling more easily
	GetEntity()->SetMaterial(gEnv->p3DEngine->GetMaterialManager()->LoadMaterial("EngineAssets/TextureMsg/DefaultSolids"));

	// Now create the physical representation of the entity
	m_pMovement->Physicalize();
}