#include "StdAfx.h"
#include "Player.h"

#include "Movement/PlayerMovement.h"
#include "Input/PlayerInput.h"
#include "View/PlayerView.h"
#include "Animations/PlayerAnimations.h"

#include "Game/GameFactory.h"
#include "Game/GameRules.h"

#include "FlowNodes/Helpers/FlowGameEntityNode.h"

#include "Entities/Gameplay/SpawnPoint.h"

#include "Entities/Gameplay/Weapons/ISimpleWeapon.h"

#include <CryRenderer/IRenderAuxGeom.h>

class CPlayerRegistrator
	: public IEntityRegistrator
	, public CPlayer::SExternalCVars
{
	virtual void Register() override
	{
		CGameFactory::RegisterGameObject<CPlayer>("Player");

		CGameFactory::RegisterGameObjectExtension<CPlayerMovement>("PlayerMovement");
		CGameFactory::RegisterGameObjectExtension<CPlayerInput>("PlayerInput");
		CGameFactory::RegisterGameObjectExtension<CPlayerView>("PlayerView");
		CGameFactory::RegisterGameObjectExtension<CPlayerAnimations>("PlayerAnimations");
		
		RegisterCVars();
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

		REGISTER_CVAR2("pl_viewDistanceFromPlayer", &m_viewDistanceFromPlayer, 10.f, VF_CHEAT, "Camera distance from player");

		m_pThirdPersonGeometry = REGISTER_STRING("pl_thirdPersonGeometry", "Objects/Characters/SampleCharacter/thirdperson.cdf", VF_CHEAT, "Sets the third person geometry to load");
		
		m_pThirdPersonMannequinContext = REGISTER_STRING("pl_thirdPersonMannequinContext", "FirstPersonCharacter", VF_CHEAT, "The name of the third person context used in Mannequin");
		m_pThirdPersonAnimationDatabase = REGISTER_STRING("pl_thirdPersonAnimationDatabase", "Animations/Mannequin/ADB/FirstPerson.adb", VF_CHEAT, "Path to the animation database file to load");
		m_pThirdPersonControllerDefinition = REGISTER_STRING("pl_thirdPersonControllerDefinition", "Animations/Mannequin/ADB/FirstPersonControllerDefinition.xml", VF_CHEAT, "Path to the controller definition file to load");
	}
};

CPlayerRegistrator g_playerRegistrator;

CPlayer::CPlayer()
	: m_pInput(nullptr)
	, m_pMovement(nullptr)
	, m_pView(nullptr)
	, m_bAlive(false)
	, m_bIsLocalClient(false)
	, m_pCurrentWeapon(nullptr)
{
}

CPlayer::~CPlayer()
{
	gEnv->pGame->GetIGameFramework()->GetIActorSystem()->RemoveActor(GetEntityId());
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
	const int requiredEvents[] = { eGFE_BecomeLocalPlayer };
	pGameObject->RegisterExtForEvents(this, requiredEvents, sizeof(requiredEvents) / sizeof(int));

	m_pMovement = static_cast<CPlayerMovement *>(GetGameObject()->AcquireExtension("PlayerMovement"));
	m_pAnimations = static_cast<CPlayerAnimations *>(GetGameObject()->AcquireExtension("PlayerAnimations"));
	m_pInput = static_cast<CPlayerInput *>(GetGameObject()->AcquireExtension("PlayerInput"));

	m_pView = static_cast<CPlayerView *>(GetGameObject()->AcquireExtension("PlayerView"));

	// Register with the actor system
	gEnv->pGame->GetIGameFramework()->GetIActorSystem()->AddActor(GetEntityId(), this);
}

void CPlayer::HandleEvent(const SGameObjectEvent &event)
{
	if (event.event == eGFE_BecomeLocalPlayer)
	{
		m_bIsLocalClient = true;
	}
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
				SetHealth(GetMaxHealth());
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
	// Spawn at first default spawner
	auto *pEntityIterator = gEnv->pEntitySystem->GetEntityIterator();
	pEntityIterator->MoveFirst();

	auto *pSpawnerClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("SpawnPoint");
	auto extensionId = gEnv->pGame->GetIGameFramework()->GetIGameObjectSystem()->GetID("SpawnPoint");

	while (!pEntityIterator->IsEnd())
	{
		IEntity *pEntity = pEntityIterator->Next();

		if (pEntity->GetClass() != pSpawnerClass)
			continue;

		auto *pGameObject = gEnv->pGame->GetIGameFramework()->GetGameObject(pEntity->GetId());
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
	if (auto *pGameObject = gEnv->pGame->GetIGameFramework()->GetGameObject(pWeaponEntity->GetId()))
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