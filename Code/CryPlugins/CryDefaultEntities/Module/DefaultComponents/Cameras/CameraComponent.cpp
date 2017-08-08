#include "StdAfx.h"
#include "CameraComponent.h"

namespace Cry
{
	namespace DefaultComponents
	{
		CCameraComponent* CCameraComponent::s_pActiveCamera = nullptr;

		void CCameraComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
		{
			// Functions
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCameraComponent::Activate, "{56EFC341-8541-4A85-9870-68F2774BDED4}"_cry_guid, "Activate");
				pFunction->SetDescription("Makes this camera active");
				pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
				componentScope.Register(pFunction);
			}
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCameraComponent::IsActive, "{5DBFFF7C-7D82-4645-ACE3-561748376568}"_cry_guid, "IsActive");
				pFunction->SetDescription("Is this camera active?");
				pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
				pFunction->BindOutput(0, 'iact', "IsActive");
				componentScope.Register(pFunction);
			}
		}

		CCameraComponent::~CCameraComponent()
		{
			if (s_pActiveCamera == this)
			{
				s_pActiveCamera = nullptr;

				if (IHmdDevice* pDevice = gEnv->pSystem->GetHmdManager()->GetHmdDevice())
				{
					pDevice->SetAsynCameraCallback(nullptr);
				}
			}
		}

		void CCameraComponent::Initialize()
		{
			if (m_bActivateOnCreate)
			{
				Activate();
			}

			m_pAudioListener = m_pEntity->GetOrCreateComponent<Cry::Audio::DefaultComponents::CListenerComponent>();
			CRY_ASSERT(m_pAudioListener != nullptr);
			m_pAudioListener->SetComponentFlags(m_pAudioListener->GetComponentFlags() | IEntityComponent::EFlags::UserAdded);
		}

		void CCameraComponent::ProcessEvent(SEntityEvent& event)
		{
			if (event.event == ENTITY_EVENT_UPDATE)
			{
				const CCamera& systemCamera = gEnv->pSystem->GetViewCamera();

				const float farPlane = gEnv->p3DEngine->GetMaxViewDistance();

				m_camera.SetFrustum(systemCamera.GetViewSurfaceX(), systemCamera.GetViewSurfaceZ(), m_fieldOfView.ToRadians(), m_nearPlane, farPlane, systemCamera.GetPixelAspectRatio());
				m_camera.SetMatrix(GetWorldTransformMatrix());

				gEnv->pSystem->SetViewCamera(m_camera);
			}
			else if (event.event == ENTITY_EVENT_START_GAME || event.event == ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED)
			{
				if (m_bActivateOnCreate && !IsActive())
				{
					Activate();
				}
			}
		}

		uint64 CCameraComponent::GetEventMask() const
		{
			uint64 bitFlags = IsActive() ? BIT64(ENTITY_EVENT_UPDATE) : 0;
			bitFlags |= BIT64(ENTITY_EVENT_START_GAME) | BIT64(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);

			return bitFlags;
		}

		bool CCameraComponent::OnAsyncCameraCallback(const HmdTrackingState& sensorState, IHmdDevice::AsyncCameraContext& context)
		{
			context.outputCameraMatrix = GetWorldTransformMatrix();

			Matrix33 orientation = Matrix33(context.outputCameraMatrix);
			Vec3 position = context.outputCameraMatrix.GetTranslation();

			context.outputCameraMatrix.AddTranslation(orientation * sensorState.pose.position);
			context.outputCameraMatrix.SetRotation33(orientation * Matrix33(sensorState.pose.orientation));

			return true;
		}
	}
}