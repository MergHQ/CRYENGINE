#include "StdAfx.h"
#include "Player.h"

#include "Movement/PlayerMovement.h"
#include "Input/PlayerInput.h"
#include "View/PlayerView.h"
#include "Animations/PlayerAnimations.h"

#include "GamePlugin.h"
#include "Game/GameRules.h"

#include "Entities/Gameplay/SpawnPoint.h"

#include "Entities/Gameplay/Weapons/ISimpleWeapon.h"

#include <CryRenderer/IRenderAuxGeom.h>

class CPlayerRegistrator
	: public IEntityRegistrator
	, public CPlayer::SExternalCVars
{
	virtual void Register() override
	{
		CGamePlugin::RegisterEntityWithDefaultComponent<CPlayer>("Player");

		CGamePlugin::RegisterEntityComponent<CPlayerMovement>("PlayerMovement");
		CGamePlugin::RegisterEntityComponent<CPlayerInput>("PlayerInput");
		CGamePlugin::RegisterEntityComponent<CPlayerView>("PlayerView");
		CGamePlugin::RegisterEntityComponent<CPlayerAnimations>("PlayerAnimations");
		
		RegisterCVars();
	}

	virtual void Unregister() override
	{
		UnregisterCVars();
	}

	void RegisterCVars()
	{
		REGISTER_CVAR2("pl_mass", &m_mass, 90.f, VF_CHEAT, "Mass of the player entity in kg");
		REGISTER_CVAR2("pl_moveSpeed", &m_moveSpeed, 20.5f, VF_CHEAT, "Speed at which the player moves");

		REGISTER_CVAR2("pl_rotationSpeedYaw", &m_rotationSpeedYaw, 0.05f, VF_CHEAT, "Speed at which the player rotates entity yaw");
		REGISTER_CVAR2("pl_rotationSpeedPitch", &m_rotationSpeedPitch, 0.05f, VF_CHEAT, "Speed at which the player rotates entity pitch");

		REGISTER_CVAR2("pl_rotationLimitsMinPitch", &m_rotationLimitsMinPitch, -0.84f, VF_CHEAT, "Minimum entity pitch limit");
		REGISTER_CVAR2("pl_rotationLimitsMaxPitch", &m_rotationLimitsMaxPitch, 1.5f, VF_CHEAT, "Maximum entity pitch limit");

		REGISTER_CVAR2("pl_eyeHeight", &m_playerEyeHeight, 0.935f, VF_CHEAT, "Height of the player's eyes from ground");

		REGISTER_CVAR2("pl_viewOffsetForward", &m_viewOffsetY, -1.5f, VF_CHEAT, "View offset along the forward axis from the player entity");
		REGISTER_CVAR2("pl_viewOffsetUp", &m_viewOffsetZ, 2.f, VF_CHEAT, "View offset along the up axis from the player entity");

		m_pThirdPersonGeometry = REGISTER_STRING("pl_thirdPersonGeometry", "Objects/Characters/SampleCharacter/thirdperson.cdf", VF_CHEAT, "Sets the third person geometry to load");
		
		m_pThirdPersonMannequinContext = REGISTER_STRING("pl_thirdPersonMannequinContext", "FirstPersonCharacter", VF_CHEAT, "The name of the third person context used in Mannequin");
		m_pThirdPersonAnimationDatabase = REGISTER_STRING("pl_thirdPersonAnimationDatabase", "Animations/Mannequin/ADB/FirstPerson.adb", VF_CHEAT, "Path to the animation database file to load");
		m_pThirdPersonControllerDefinition = REGISTER_STRING("pl_thirdPersonControllerDefinition", "Animations/Mannequin/ADB/FirstPersonControllerDefinition.xml", VF_CHEAT, "Path to the controller definition file to load");
	}

	void UnregisterCVars()
	{
		IConsole* pConsole = gEnv->pConsole;
		if (pConsole)
		{
			pConsole->UnregisterVariable("pl_mass");
			pConsole->UnregisterVariable("pl_moveSpeed");
			pConsole->UnregisterVariable("pl_rotationSpeedYaw");
			pConsole->UnregisterVariable("pl_rotationSpeedPitch");
			pConsole->UnregisterVariable("pl_rotationLimitsMinPitch");
			pConsole->UnregisterVariable("pl_rotationLimitsMaxPitch");
			pConsole->UnregisterVariable("pl_eyeHeight");
			pConsole->UnregisterVariable("pl_viewOffsetForward");
			pConsole->UnregisterVariable("pl_viewOffsetUp");
			pConsole->UnregisterVariable("pl_thirdPersonGeometry");
			pConsole->UnregisterVariable("pl_thirdPersonMannequinContext");
			pConsole->UnregisterVariable("pl_thirdPersonAnimationDatabase");
			pConsole->UnregisterVariable("pl_thirdPersonControllerDefinition");
		}
	}
};

CPlayerRegistrator g_playerRegistrator;

CPlayer::CPlayer()
	: m_pInput(nullptr)
	, m_pMovement(nullptr)
	, m_pView(nullptr)
	, m_bAlive(false)
	, m_pCurrentWeapon(nullptr)
{
}

CPlayer::~CPlayer()
{
	gEnv->pGameFramework->GetIActorSystem()->RemoveActor(GetEntityId());
}

const CPlayer::SExternalCVars &CPlayer::GetCVars() const
{
	return g_playerRegistrator;
}

bool CPlayer::Init(IGameObject *pGameObject)
{
	SetGameObject(pGameObject);

	return pGameObject->BindToNetwork();
}

void CPlayer::PostInit(IGameObject *pGameObject)
{
	m_pMovement = static_cast<CPlayerMovement *>(GetGameObject()->AcquireExtension("PlayerMovement"));
	m_pAnimations = static_cast<CPlayerAnimations *>(GetGameObject()->AcquireExtension("PlayerAnimations"));
	m_pInput = static_cast<CPlayerInput *>(GetGameObject()->AcquireExtension("PlayerInput"));

	m_pView = static_cast<CPlayerView *>(GetGameObject()->AcquireExtension("PlayerView"));

	// Register with the actor system
	gEnv->pGameFramework->GetIActorSystem()->AddActor(GetEntityId(), this);
}

void CPlayer::ProcessEvent(SEntityEvent& event)
{
	switch (event.event)
	{
	case ENTITY_EVENT_RESET:
	{
		switch (event.nParam[0])
		{
		case 0: // Game ends
			m_pCurrentWeapon = nullptr;
			break;
		case 1: // Game starts
				// Make sure to revive player when respawning in Editor
			SetHealth(GetMaxHealth());
			// Create rifle
			if (!m_pCurrentWeapon) CreateWeapon("Rifle");
			break;
		default:
			break;
		}
	}
	break;
	case ENTITY_EVENT_HIDE:
	{
		// Hide the weapon too
		if (m_pCurrentWeapon != nullptr)
		{
			m_pCurrentWeapon->GetEntity()->Hide(true);
		}
	}
	break;
	case ENTITY_EVENT_UNHIDE:
	{
		// Unhide the weapon too
		if (m_pCurrentWeapon != nullptr)
		{
			m_pCurrentWeapon->GetEntity()->Hide(false);
		}
	}
	break;
	}
}

void CPlayer::SetHealth(float health)
{
	// Find a spawn point and move the entity there
	SelectSpawnPoint();

	// Note that this implementation does not handle the concept of death, SetHealth(0) will still revive the player.
	if (m_bAlive)
		return;

	m_bAlive = true;

	// Unhide the entity in case hidden by the Editor
	GetEntity()->Hide(false);

	// Make sure that the player spawns upright
	GetEntity()->SetWorldTM(Matrix34::Create(Vec3(1, 1, 1), IDENTITY, GetEntity()->GetWorldPos()));

	// Set the player geometry, this also triggers physics proxy creation
	SetPlayerModel();

	// Notify input that the player respawned
	m_pInput->OnPlayerRespawn();

	// Spawn the player with a weapon
	if (m_pCurrentWeapon == nullptr)
	{
		CreateWeapon("Rifle");
	}
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
	auto extensionId = gEnv->pGameFramework->GetIGameObjectSystem()->GetID("SpawnPoint");

	while (!pEntityIterator->IsEnd())
	{
		IEntity *pEntity = pEntityIterator->Next();

		if (pEntity->GetClass() != pSpawnerClass)
			continue;

		auto *pGameObject = gEnv->pGameFramework->GetGameObject(pEntity->GetId());
		if (pGameObject == nullptr)
			continue;

		auto *pSpawner = static_cast<CSpawnPoint *>(pGameObject->QueryExtension(extensionId));
		if (pSpawner == nullptr)
			continue;

		pSpawner->SpawnEntity(*GetEntity());

		break;
	}
}

void CPlayer::SetPlayerModel()
{
	// Load the third person model
	GetEntity()->LoadCharacter(eGeometry_ThirdPerson, GetCVars().m_pThirdPersonGeometry->GetString());
	
	// Do the same for animations so that Mannequin data can be initialized
	m_pAnimations->OnPlayerModelChanged();

	// Now create the physical representation of the entity
	m_pMovement->Physicalize();
}

void CPlayer::CreateWeapon(const char *name)
{
	SEntitySpawnParams spawnParams;

	// Set the class of the entity we want to create, e.g. "Rifle"
	spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);

	// Now spawn the entity via the entity system
	// Note that the game object extension (CRifle in this example) will be acquired automatically.
	IEntity *pWeaponEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams);
	CRY_ASSERT(pWeaponEntity != nullptr);
	
	// Now acquire the game object for this entity
	if (auto *pGameObject = gEnv->pGameFramework->GetGameObject(pWeaponEntity->GetId()))
	{
		// Obtain our ISimpleWeapon implementation, based on IGameObjectExtension
		if (auto *pWeapon = pGameObject->QueryExtension(name))
		{
			// Set the equipped weapon
			m_pCurrentWeapon = static_cast<ISimpleWeapon *>(pWeapon);
		}
		else
		{
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to query game object extension for weapon %s!", name);
		}
	}
	else
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Spawned weapon of type %s but failed to get game object!", name);
	}
}