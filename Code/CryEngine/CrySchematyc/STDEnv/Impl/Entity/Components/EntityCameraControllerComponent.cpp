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

#include <CryGame/IGameFramework.h>
#include <IViewSystem.h>

namespace Schematyc
{
	void CEntityOrbitCameraControllerComponent::SProperties::Serialize(Serialization::IArchive& archive)
	{
		archive(yaw, "yaw", "Yaw");
		archive(pitch, "pitch", "Pitch");
		archive(distanceToTarget, "distance", "Distance");
		archive(fov, "fov", "FOV");
		archive(bActive, "active", "Active");
	}

	//////////////////////////////////////////////////////////////////////////

	CEntityOrbitCameraControllerComponent::~CEntityOrbitCameraControllerComponent()
	{
		if (m_viewId != -1)
		{
			gEnv->pGameFramework->GetIViewSystem()->RemoveView(m_viewId);
			IEntity& cameraTarget = EntityUtils::GetEntity(*this);
			if(IGameObject* pGameObject = gEnv->pGameFramework->GetGameObject(cameraTarget.GetId()))
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
			if (m_viewId == -1)  //create view
			{
				IViewSystem* pViewSystem = gEnv->pGameFramework->GetIViewSystem();
				IView* pView = pViewSystem->CreateView();
				m_viewId = pViewSystem->GetViewId(pView);

				IEntity& cameraTarget = EntityUtils::GetEntity(*this);
				auto* pGameObject = gEnv->pGameFramework->GetIGameObjectSystem()->CreateGameObjectForEntity(cameraTarget.GetId());
				pView->LinkTo(pGameObject);
				pGameObject->CaptureView(this);
			}

			const SProperties* pProperties = static_cast<const SProperties*>(CComponent::GetProperties());
			m_rotation.x = pProperties->yaw;
			m_rotation.y = pProperties->roll;
			m_rotation.z = pProperties->pitch;
			m_distance = pProperties->distanceToTarget;
			m_fov = DEG2RAD(pProperties->fov);
			SetActive(pProperties->bActive);
		}
	}

	void CEntityOrbitCameraControllerComponent::QueueUpdate()
	{
		m_bNeedUpdate = true;
	}

	SGUID CEntityOrbitCameraControllerComponent::ReflectSchematycType(CTypeInfo<CEntityOrbitCameraControllerComponent>& typeInfo)
	{
		return "AED0875B-0B8A-4961-AE6F-CE613956529F"_schematyc_guid;
	}

	void CEntityOrbitCameraControllerComponent::Register(IEnvRegistrar& registrar)
	{
		CEnvRegistrationScope scope = registrar.Scope(g_entityClassGUID);
		{
			auto pComponent = SCHEMATYC_MAKE_ENV_COMPONENT(CEntityOrbitCameraControllerComponent, "OrbitCameraController");
			pComponent->SetAuthor(g_szCrytek);
			pComponent->SetDescription("Orbit camera controller component");
			pComponent->SetIcon("icons:schematyc/camera.ico");
			pComponent->SetFlags(EEnvComponentFlags::Singleton);
			pComponent->SetProperties(CEntityOrbitCameraControllerComponent::SProperties());
			scope.Register(pComponent);

			CEnvRegistrationScope componentScope = registrar.Scope(pComponent->GetGUID());
			// Functions
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityOrbitCameraControllerComponent::SetDistance, "8D75BD31-8BD6-4468-A531-630B11790FB4"_schematyc_guid, "SetDistance");
				pFunction->SetAuthor(g_szCrytek);
				pFunction->SetDescription("positions the camera in the specified distance to the target object");
				pFunction->SetFlags(EEnvFunctionFlags::Construction);
				pFunction->BindInput(1, 'dis', "Distance");
				pFunction->BindInput(2, 'rel', "Relative", "Should the distance be relative to the current distance");
				componentScope.Register(pFunction);
			}
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityOrbitCameraControllerComponent::SetYaw, "567810FF-C2BF-4B9F-8EA2-6106320BA083"_schematyc_guid, "SetYaw");
				pFunction->SetAuthor(g_szCrytek);
				pFunction->SetDescription("positions the camera in the specified (horizontal) angle to the target object");
				pFunction->SetFlags(EEnvFunctionFlags::Construction);
				pFunction->BindInput(1, 'ang', "Angle", "Angle in degree");
				pFunction->BindInput(2, 'rel', "Relative", "Should the angle be relative to the current yaw");
				componentScope.Register(pFunction);
			} 
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityOrbitCameraControllerComponent::SetPitch, "8C5D5DBA-944F-4E30-AB38-8C283F0C5FD1"_schematyc_guid, "SetPitch");
				pFunction->SetAuthor(g_szCrytek);
				pFunction->SetDescription("positions the camera in the specified (vertical) angle to the target object");
				pFunction->SetFlags(EEnvFunctionFlags::Construction);
				pFunction->BindInput(1, 'ang', "Angle", "Angle in degree");
				pFunction->BindInput(2, 'rel', "Relative", "Should the angle be relative to the current pitch");
				componentScope.Register(pFunction);
			}
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityOrbitCameraControllerComponent::SetFov, "EAA6506E-41D0-4BD0-9C95-BD94CD34462E"_schematyc_guid, "SetFov");
				pFunction->SetAuthor(g_szCrytek);
				pFunction->SetDescription("Sets the FOV of the camera");
				pFunction->SetFlags(EEnvFunctionFlags::Construction);
				pFunction->BindInput(1, 'fov', "FOV", "FOV in degree");
				pFunction->BindInput(2, 'rel', "Relative", "Should the FOV relative to the current FOV");
				componentScope.Register(pFunction);
			}
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityOrbitCameraControllerComponent::SetRoll, "2812ED72-9AA2-4057-9955-0E6A19D756B7"_schematyc_guid, "SetRoll");
				pFunction->SetAuthor(g_szCrytek);
				pFunction->SetDescription("Sets the Roll angle of the camera");
				pFunction->SetFlags(EEnvFunctionFlags::Construction);
				pFunction->BindInput(1, 'rol', "Roll", "Roll in degree");
				pFunction->BindInput(2, 'rel', "Relative", "Should the angle be relative to the current Roll");
				componentScope.Register(pFunction);
			}
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityOrbitCameraControllerComponent::SetActive, "B1385617-98F6-48F2-BCA0-D72110553614"_schematyc_guid, "SetActive");
				pFunction->SetAuthor(g_szCrytek);
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
		if (bRelative)
		{
			if (angle != 0.0f)
			{
				m_rotation.z += DEG2RAD(angle);
				QueueUpdate();
			}
		}
		else
		{
			if (m_rotation.z != angle)
			{
				m_rotation.z = DEG2RAD(angle);
				QueueUpdate();
			}
		}
	}
	
	void CEntityOrbitCameraControllerComponent::SetPitch(float angle, bool bRelative)
	{
		if (bRelative)
		{
			if (angle != 0.0f)
			{
				m_rotation.x += DEG2RAD(angle);
				QueueUpdate();
			}
		}
		else
		{
			if (m_rotation.x != angle)
			{
				m_rotation.x = DEG2RAD(angle);
				QueueUpdate();
			}
		}
	}


	void CEntityOrbitCameraControllerComponent::SetRoll(float angle, bool bRelative)
	{
		if (bRelative)
		{
			if (angle != 0.0f)
			{
				m_rotation.y += DEG2RAD(angle);
				QueueUpdate();
			}
		}
		else
		{
			if (m_rotation.y != angle)
			{
				m_rotation.y = DEG2RAD(angle);
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
		if (m_viewId == -1)  //preview-object
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
		if (m_bNeedUpdate)
		{
			IEntity& cameraTarget = EntityUtils::GetEntity(*this);
			Vec3 targetObjectPos = cameraTarget.GetWorldPos();
			Matrix34 mat = Matrix34::CreateRotationXYZ(m_rotation);
			Vec3 lookDir = mat.GetColumn1();
			params.position = targetObjectPos - lookDir * m_distance;
			params.rotation = Quat(mat);
			params.fov = m_fov;

			m_bNeedUpdate = false;
		}
	}
} //Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::CEntityOrbitCameraControllerComponent::Register)
