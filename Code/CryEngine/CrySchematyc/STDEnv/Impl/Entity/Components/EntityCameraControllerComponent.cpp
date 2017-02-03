// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "EntityCameraControllerComponent.h"
#include "AutoRegister.h"
#include "STDModules.h"

#include <Schematyc/Entity/EntityUtils.h>
#include <Schematyc/Entity/EntityClasses.h>
#include <Schematyc/Types/ResourceTypes.h>
#include <CryEntitySystem/IEntityComponent.h>
#include <CryMath/Cry_Camera.h>
#include <CrySerialization/Decorators/Range.h>

#include <CryGame/IGameFramework.h>
#include <IViewSystem.h>

namespace Schematyc
{
static const float g_maxCameraPitchDeg = 80.0f;
static const float g_maxCameraPitchRad = DEG2RAD(g_maxCameraPitchDeg);

//////////////////////////////////////////////////////////////////////////

CEntityOrbitCameraControllerComponent::~CEntityOrbitCameraControllerComponent()
{
	if (m_viewId != -1)
	{
		gEnv->pGameFramework->GetIViewSystem()->RemoveView(m_viewId);
		IEntity& cameraTarget = EntityUtils::GetEntity(*this);
		if (IGameObject* pGameObject = gEnv->pGameFramework->GetGameObject(cameraTarget.GetId()))
		{
			pGameObject->ReleaseView(this);
		}
		m_viewId = -1;
	}
}

bool CEntityOrbitCameraControllerComponent::Init()
{
	return true;
}

void CEntityOrbitCameraControllerComponent::Run(ESimulationMode simulationMode)
{
	if (simulationMode == ESimulationMode::Game)
	{
		if (m_viewId == -1)    //create view
		{
			IViewSystem* pViewSystem = gEnv->pGameFramework->GetIViewSystem();
			IView* pView = pViewSystem->CreateView();
			m_viewId = pViewSystem->GetViewId(pView);

			IEntity& cameraTarget = EntityUtils::GetEntity(*this);
			auto* pGameObject = gEnv->pGameFramework->GetIGameObjectSystem()->CreateGameObjectForEntity(cameraTarget.GetId());
			pView->LinkTo(pGameObject);
			pGameObject->CaptureView(this);
		}

		m_rotation.x = DEG2RAD(m_pitch);
		m_rotation.y = DEG2RAD(m_roll);
		m_rotation.z = DEG2RAD(m_yaw);
		m_fov = DEG2RAD(m_fov);
		SetActive(m_bActive);
	}
}

void CEntityOrbitCameraControllerComponent::QueueUpdate()
{
	m_bNeedUpdate = true;
}

void CEntityOrbitCameraControllerComponent::ReflectType(CTypeDesc<CEntityOrbitCameraControllerComponent>& desc)
{
	desc.SetGUID("AED0875B-0B8A-4961-AE6F-CE613956529F"_schematyc_guid);
	desc.SetLabel("OrbitCameraController");
	desc.SetDescription("Orbit camera controller component");
	desc.SetIcon("icons:schematyc/camera.ico");
	desc.SetComponentFlags(EComponentFlags::Singleton);
	desc.AddMember(&CEntityOrbitCameraControllerComponent::m_yaw, 'yaw', "yaw", "Yaw", "Yaw (degrees)", 0.0f);
	desc.AddMember(&CEntityOrbitCameraControllerComponent::m_pitch, 'pit', "pitch", "Pitch", "Pitch (degrees)", 45.0f);
	desc.AddMember(&CEntityOrbitCameraControllerComponent::m_distance, 'dist', "distance", "Distance", "Orbit distance from entity", 10.0f);
	desc.AddMember(&CEntityOrbitCameraControllerComponent::m_fov, 'fov', "fov", "Field of View", "Field of view (degrees)", 60.0f);
	desc.AddMember(&CEntityOrbitCameraControllerComponent::m_bActive, 'act', "bActive", "Active", "Activate camera", true);
	desc.AddMember(&CEntityOrbitCameraControllerComponent::m_bAlwaysUpdate, 'updt', "bAlwaysUpdate", "Always Update", "Update camera every frame", false);
}

void CEntityOrbitCameraControllerComponent::Register(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(g_entityClassGUID);
	{
		CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CEntityOrbitCameraControllerComponent));
		// Functions
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityOrbitCameraControllerComponent::SetDistance, "8D75BD31-8BD6-4468-A531-630B11790FB4"_schematyc_guid, "SetDistance");
			pFunction->SetDescription("positions the camera in the specified distance to the target object");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'dis', "Distance");
			pFunction->BindInput(2, 'rel', "Relative", "Should the distance be relative to the current distance");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityOrbitCameraControllerComponent::SetYaw, "567810FF-C2BF-4B9F-8EA2-6106320BA083"_schematyc_guid, "SetYaw");
			pFunction->SetDescription("positions the camera in the specified (horizontal) angle to the target object");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'ang', "Angle", "Angle in degree");
			pFunction->BindInput(2, 'rel', "Relative", "Should the angle be relative to the current yaw");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityOrbitCameraControllerComponent::SetPitch, "8C5D5DBA-944F-4E30-AB38-8C283F0C5FD1"_schematyc_guid, "SetPitch");
			pFunction->SetDescription("positions the camera in the specified (vertical) angle to the target object");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'ang', "Angle", "Angle in degree");
			pFunction->BindInput(2, 'rel', "Relative", "Should the angle be relative to the current pitch");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityOrbitCameraControllerComponent::SetFov, "EAA6506E-41D0-4BD0-9C95-BD94CD34462E"_schematyc_guid, "SetFov");
			pFunction->SetDescription("Sets the FOV of the camera");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'fov', "FOV", "FOV in degree");
			pFunction->BindInput(2, 'rel', "Relative", "Should the FOV relative to the current FOV");
			componentScope.Register(pFunction);
		}
		/*{ // #SchematycTODO : Temporarily disabled until we can verify roll is working as expected.
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityOrbitCameraControllerComponent::SetRoll, "2812ED72-9AA2-4057-9955-0E6A19D756B7"_schematyc_guid, "SetRoll");
			pFunction->SetDescription("Sets the Roll angle of the camera");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'rol', "Roll", "Roll in degree");
			pFunction->BindInput(2, 'rel', "Relative", "Should the angle be relative to the current Roll");
			componentScope.Register(pFunction);
		}*/
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityOrbitCameraControllerComponent::SetActive, "B1385617-98F6-48F2-BCA0-D72110553614"_schematyc_guid, "SetActive");
			pFunction->SetDescription("Activates/Deactivates the camera");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'ena', "value", "active this camera");
			componentScope.Register(pFunction);
		}
	}
}

void CEntityOrbitCameraControllerComponent::SetDistance(float distance, bool bRelative)
{
	const float newValue = (bRelative) ? m_distance + distance : distance;
	if (newValue > FLT_EPSILON && m_distance != newValue)
	{
		m_distance = newValue;
		QueueUpdate();
	}
}

void CEntityOrbitCameraControllerComponent::SetYaw(float angle, bool bRelative)
{
	angle = DEG2RAD(angle);
	if (bRelative)
	{
		if (angle != 0.0f)
		{
			m_rotation.z += angle;
			QueueUpdate();
		}
	}
	else
	{
		if (m_rotation.z != angle)
		{
			m_rotation.z = angle;
			QueueUpdate();
		}
	}
}

void CEntityOrbitCameraControllerComponent::SetPitch(float angle, bool bRelative)
{
	angle = DEG2RAD(angle);
	if (bRelative)
	{
		if (angle != 0.0f)
		{
			m_rotation.x = crymath::clamp(m_rotation.x + angle, -g_maxCameraPitchRad, g_maxCameraPitchRad);
			QueueUpdate();
		}
	}
	else
	{
		if (m_rotation.x != angle)
		{
			m_rotation.x = crymath::clamp(angle, -g_maxCameraPitchRad, g_maxCameraPitchRad);
			QueueUpdate();
		}
	}
}

void CEntityOrbitCameraControllerComponent::SetRoll(float angle, bool bRelative)
{
	angle = DEG2RAD(angle);
	if (bRelative)
	{
		if (angle != 0.0f)
		{
			m_rotation.y += angle;
			QueueUpdate();
		}
	}
	else
	{
		if (m_rotation.y != angle)
		{
			m_rotation.y = angle;
			QueueUpdate();
		}
	}
}

void CEntityOrbitCameraControllerComponent::SetFov(float fov, bool bRelative)
{
	constexpr float minFov = 0.05f;
	constexpr float maxFov = 3.1f;

	if (bRelative)
	{
		if (fov != 0.0f)
		{
			m_fov += DEG2RAD(fov);
			if (m_fov < minFov)
				m_fov = minFov;
			if (m_fov > maxFov)
				m_fov = maxFov;
			QueueUpdate();
		}
	}
	else
	{
		const float newFov = DEG2RAD(fov);
		if (m_fov != newFov)
		{
			m_fov = newFov;
			if (m_fov < minFov)
				m_fov = minFov;
			if (m_fov > maxFov)
				m_fov = maxFov;
			QueueUpdate();
		}
	}
}

void CEntityOrbitCameraControllerComponent::SetActive(bool bEnable)
{
	if (m_viewId == -1)    //preview-object
		return;

	if (bEnable)
	{
		IViewSystem* pViewSystem = gEnv->pGameFramework->GetIViewSystem();
		QueueUpdate();
		pViewSystem->SetActiveView(m_viewId);
	}
}

void CEntityOrbitCameraControllerComponent::UpdateView(SViewParams& params)
{
	if (m_bAlwaysUpdate || m_bNeedUpdate)
	{
		IEntity& entity = EntityUtils::GetEntity(*this);
		const Vec3 target = entity.GetWorldPos();
		const Quat rot = Quat::CreateRotationXYZ(Ang3(-m_rotation.x, 0.0f, m_rotation.z));
		const Vec3 forward = rot.GetColumn1();

		params.position = target - (forward * m_distance);
		params.rotation = Quat::CreateRotationY(m_rotation.y) * rot;
		params.fov = m_fov;

		m_bNeedUpdate = false;
	}
}
} //Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::CEntityOrbitCameraControllerComponent::Register)
