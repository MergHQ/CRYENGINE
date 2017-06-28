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
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCameraComponent::OverrideAudioListenerTransform, "{46FA7961-95AF-4BF3-B7E8-A67ECE233EE3}"_cry_guid, "OverrideAudioListenerTransform");
				pFunction->SetDescription("Overrides the transformation of this camera's audio listener. Using this function will set Automatic Audio Listener to false, meaning that the audio listener will stay at the overridden position until this function is called again.");
				pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
				pFunction->BindInput(1, 'tran', "Transform");
				componentScope.Register(pFunction);
			}
		}

		void CCameraComponent::ReflectType(Schematyc::CTypeDesc<CCameraComponent>& desc)
		{
			desc.SetGUID(CCameraComponent::IID());
			desc.SetEditorCategory("Cameras");
			desc.SetLabel("Camera");
			desc.SetDescription("Represents a camera that can be activated to render to screen");
			desc.SetIcon("icons:General/Camera.ico");
			desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });
			
			desc.AddMember(&CCameraComponent::m_bActivateOnCreate, 'actv', "Active", "Active", "Whether or not this camera should be activated on component creation", true);
			desc.AddMember(&CCameraComponent::m_nearPlane, 'near', "NearPlane", "Near Plane", nullptr, 0.25f);
			desc.AddMember(&CCameraComponent::m_fieldOfView, 'fov', "FieldOfView", "Field of View", nullptr, 75.0_degrees);

			desc.AddMember(&CCameraComponent::m_bAutomaticAudioListenerPosition, 'audi', "AutoAudioListenerPos", "Automatic Audio Listener", "If true, automatically moves the audio listener with the entity.", true);
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

			if (m_pAudioListener != nullptr)
			{
				gEnv->pEntitySystem->RemoveEntityEventListener(m_pAudioListener->GetId(), ENTITY_EVENT_DONE, this);
				gEnv->pEntitySystem->RemoveEntity(m_pAudioListener->GetId(), true);
				m_pAudioListener = nullptr;
			}
		}

		void CCameraComponent::Initialize()
		{
			if (m_bActivateOnCreate)
			{
				Activate();
			}
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

				if (m_bAutomaticAudioListenerPosition)
				{
					// Make sure we update the audio listener position
					m_pAudioListener->SetWorldTM(m_camera.GetMatrix());
				}
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

		void CCameraComponent::OnEntityEvent(IEntity* pEntity, SEntityEvent& event)
		{
			switch (event.event)
			{
			case ENTITY_EVENT_DONE:
			{
				// In case something destroys our listener entity before we had the chance to remove it.
				if ((m_pAudioListener != nullptr) && (pEntity->GetId() == m_pAudioListener->GetId()))
				{
					gEnv->pEntitySystem->RemoveEntityEventListener(m_pAudioListener->GetId(), ENTITY_EVENT_DONE, this);
					m_pAudioListener = nullptr;
				}

				break;
			}
			}
		}
	}
}