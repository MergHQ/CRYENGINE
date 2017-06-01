#include "StdAfx.h"
#include "Player.h"
#include "SpawnPoint.h"

#include <CryRenderer/IRenderAuxGeom.h>
#include <CryInput/IHardwareMouse.h>

void CPlayerComponent::Initialize()
{
	// Create the camera component, will automatically update the viewport every frame
	m_pCameraComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCameraComponent>();
	// The character controller is responsible for maintaining player physics and animations
	m_pCharacterController = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCharacterControllerComponent>();
	// Get the input component, wraps access to action mapping so we can easily get callbacks when inputs are m_pEntity
	m_pInputComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CInputComponent>();
	// Get and initialize the pathfinding component
	m_pPathfindingComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CPathfindingComponent>();

	m_pPathfindingComponent->SetMovementRecommendationCallback( [this](const Vec3& recommendedVelocity)
	{
		m_pCharacterController->SetVelocity(recommendedVelocity);
	});

	// Register the shoot action
	m_pInputComponent->RegisterAction("player", "walkto", [this](int activationMode, float value)
	{
		if (m_pCursorEntity != nullptr && activationMode == eAAM_OnPress)
		{
			if (m_pPathfindingComponent->IsProcessingRequest())
			{
				m_pPathfindingComponent->CancelCurrentRequest();
			}

			m_pPathfindingComponent->RequestMoveTo(m_pCursorEntity->GetWorldPos());
		}
	});

	// Bind the walkto action to left mouse click
	m_pInputComponent->BindAction("player", "walkto", eAID_KeyboardMouse, EKeyId::eKI_Mouse1);

	// Spawn the cursor
	SpawnCursorEntity();
}

uint64 CPlayerComponent::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_RESET) | BIT64(ENTITY_EVENT_START_GAME) | BIT64(ENTITY_EVENT_UPDATE);
}

void CPlayerComponent::ProcessEvent(SEntityEvent& event)
{
	switch (event.event)
	{
	case ENTITY_EVENT_RESET:
	{
		// Check if we're entering game mode
		if (event.nParam[0] == 1)
		{
			SpawnCursorEntity();
		}
		else
		{
			// Removed by Sandbox
			m_pCursorEntity = nullptr;
		}
	}
	break;
	case ENTITY_EVENT_START_GAME:
	{
		// Revive the entity when gameplay starts
		Revive();
	}
	break;
	case ENTITY_EVENT_UPDATE:
	{
		SEntityUpdateContext* pCtx = (SEntityUpdateContext*)event.nParam[0];

		// Update the in-world cursor position
		UpdateCursor(pCtx->fFrameTime);

		// Update the animation state of the character
		UpdateAnimation(pCtx->fFrameTime);

		// Update the camera component offset
		UpdateCamera(pCtx->fFrameTime);
	}
	break;
	}
}

void CPlayerComponent::SpawnCursorEntity()
{
	if (m_pCursorEntity)
	{
		gEnv->pEntitySystem->RemoveEntity(m_pCursorEntity->GetId());
	}

	SEntitySpawnParams spawnParams;
	// No need for a special class!
	spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();

	// Spawn the cursor
	m_pCursorEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams);

	// Load geometry
	const int geometrySlot = 0;
	m_pCursorEntity->LoadGeometry(geometrySlot, "Objects/Default/primitive_sphere.cgf");

	// Scale the cursor down a bit
	m_pCursorEntity->SetScale(Vec3(0.1f));
	m_pCursorEntity->SetViewDistRatio(255);

	// Load the custom cursor material
	auto *pCursorMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial("Materials/cursor");
	m_pCursorEntity->SetMaterial(pCursorMaterial);
}

void CPlayerComponent::UpdateAnimation(float frameTime)
{
	ICharacterInstance *pCharacter = m_pCharacterController->GetCharacter();

	// Update the Mannequin tags
	m_pCharacterController->SetTagWithId(m_rotateTagId, m_pCharacterController->IsTurning());
	m_pCharacterController->SetTagWithId(m_walkTagId, m_pCharacterController->IsWalking());

	if (m_pCharacterController->IsWalking())
	{
		Quat newRotation = Quat::CreateRotationVDir(m_pCharacterController->GetMoveDirection());

		Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(newRotation));

		// We only want to affect Z-axis rotation, zero pitch and roll
		ypr.y = 0;
		ypr.z = 0;

		// Re-calculate the quaternion based on the corrected yaw
		newRotation = Quat(CCamera::CreateOrientationYPR(ypr));

		// Send updated transform to the entity, only orientation changes
		m_pEntity->SetPosRotScale(m_pEntity->GetWorldPos(), newRotation, Vec3(1, 1, 1));
	}
}

void CPlayerComponent::UpdateCamera(float frameTime)
{
	// Start with rotating the camera isometric style
	Matrix34 localTransform = IDENTITY;
	localTransform.SetRotation33(Matrix33(m_pEntity->GetWorldRotation().GetInverted()) * Matrix33::CreateRotationXYZ(Ang3(DEG2RAD(-45), 0, DEG2RAD(-45))));

	const float viewDistanceFromPlayer = 10.f;

	// Offset the player along the forward axis (normally back)
	// Also offset upwards
	localTransform.SetTranslation(-localTransform.GetColumn1() * viewDistanceFromPlayer);

	m_pCameraComponent->SetLocalTransform(CryTransform::CTransform(localTransform));
}

void CPlayerComponent::UpdateCursor(float frameTime)
{
	float mouseX, mouseY;
	gEnv->pHardwareMouse->GetHardwareMouseClientPosition(&mouseX, &mouseY);

	// Invert mouse Y
	mouseY = gEnv->pRenderer->GetHeight() - mouseY;

	Vec3 vPos0(0, 0, 0);
	gEnv->pRenderer->UnProjectFromScreen(mouseX, mouseY, 0, &vPos0.x, &vPos0.y, &vPos0.z);

	Vec3 vPos1(0, 0, 0);
	gEnv->pRenderer->UnProjectFromScreen(mouseX, mouseY, 1, &vPos1.x, &vPos1.y, &vPos1.z);

	Vec3 vDir = vPos1 - vPos0;
	vDir.Normalize();

	const auto rayFlags = rwi_stop_at_pierceable | rwi_colltype_any;
	ray_hit hit;

	if (gEnv->pPhysicalWorld->RayWorldIntersection(vPos0, vDir * gEnv->p3DEngine->GetMaxViewDistance(), ent_all, rayFlags, &hit, 1))
	{
		if (m_pCursorEntity != nullptr)
		{
			m_pCursorEntity->SetPosRotScale(hit.pt, IDENTITY, m_pCursorEntity->GetScale());
		}
	}
}

void CPlayerComponent::Revive()
{
	// Find a spawn point and move the entity there
	SpawnAtSpawnPoint();

	// Unhide the entity in case hidden by the Editor
	GetEntity()->Hide(false);

	// Make sure that the player spawns upright
	GetEntity()->SetWorldTM(Matrix34::Create(Vec3(1, 1, 1), IDENTITY, GetEntity()->GetWorldPos()));

	// Set the player geometry, this also triggers physics proxy creation
	m_pCharacterController->SetControllerDefinitionFile("Animations/Mannequin/ADB/FirstPersonControllerDefinition.xml");
	m_pCharacterController->SetMannequinAnimationDatabaseFile("Animations/Mannequin/ADB/FirstPerson.adb");
	m_pCharacterController->SetCharacterFile("Objects/Characters/SampleCharacter/thirdperson.cdf");

	// Disable movement coming from the animation (root joint offset), we control this entirely via physics
	m_pCharacterController->SetAnimationDrivenMotion(false);

	m_pCharacterController->Initialize();

	m_pCharacterController->ActivateContext("FirstPersonCharacter");

	// Queue the idle fragment to start playing immediately on next update
	m_pCharacterController->QueueFragment("Idle");

	// Acquire tag identifiers to avoid doing so each update
	m_rotateTagId = m_pCharacterController->GetTagId("Rotate");
	m_walkTagId = m_pCharacterController->GetTagId("Walk");
}

void CPlayerComponent::SpawnAtSpawnPoint()
{
	// Spawn at first default spawner
	auto *pEntityIterator = gEnv->pEntitySystem->GetEntityIterator();
	pEntityIterator->MoveFirst();

	while (!pEntityIterator->IsEnd())
	{
		IEntity *pEntity = pEntityIterator->Next();

		if (auto* pSpawner = pEntity->GetComponent<CSpawnPointComponent>())
		{
			pSpawner->SpawnEntity(m_pEntity);
			break;
		}
	}
}