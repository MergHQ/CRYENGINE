#pragma once

#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Reflection/TypeDesc.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>

#include <CrySystem/VR/IHMDDevice.h>
#include <CrySystem/VR/IHMDManager.h>
#include "../Audio/ListenerComponent.h"
#include <CryExtension/ICryPluginManager.h>

#include "PluginDll.h"

namespace Cry
{
	namespace DefaultComponents
	{
		class CCameraComponent
			: public IEntityComponent
			, public IHmdDevice::IAsyncCameraCallback
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

			// IEntityComponent
			virtual void Initialize() override
			{
				m_pCameraManager->AddCamera(this);

				m_pAudioListener = m_pEntity->GetOrCreateComponent<Cry::Audio::DefaultComponents::CListenerComponent>();
				CRY_ASSERT(m_pAudioListener != nullptr);
				m_pAudioListener->SetComponentFlags(m_pAudioListener->GetComponentFlags() | IEntityComponent::EFlags::UserAdded);

				if (m_bActivateOnCreate)
				{
					Activate();
				}
			}

			virtual void ProcessEvent(SEntityEvent& event) override
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

			virtual uint64 GetEventMask() const override
			{
				uint64 bitFlags = IsActive() ? BIT64(ENTITY_EVENT_UPDATE) : 0;
				bitFlags |= BIT64(ENTITY_EVENT_START_GAME) | BIT64(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);

				return bitFlags;
			}
			// ~IEntityComponent

			// IAsyncCameraCallback
			virtual bool OnAsyncCameraCallback(const HmdTrackingState& sensorState, IHmdDevice::AsyncCameraContext& context) override
			{
				context.outputCameraMatrix = GetWorldTransformMatrix();

				Matrix33 orientation = Matrix33(context.outputCameraMatrix);
				Vec3 position = context.outputCameraMatrix.GetTranslation();

				context.outputCameraMatrix.AddTranslation(orientation * sensorState.pose.position);
				context.outputCameraMatrix.SetRotation33(orientation * Matrix33(sensorState.pose.orientation));

				return true;
			}
			// ~IAsyncCameraCallback

		public:
			CCameraComponent()
			{
				m_pCameraManager = gEnv->pSystem->GetIPluginManager()->QueryPlugin<IPlugin_CryDefaultEntities>()->GetICameraManager();
			}

			virtual ~CCameraComponent() override
			{
				m_pCameraManager->RemoveCamera(this);

				if (IHmdDevice* pDevice = gEnv->pSystem->GetHmdManager()->GetHmdDevice())
				{
					pDevice->SetAsynCameraCallback(nullptr);
				}
			}

			static void ReflectType(Schematyc::CTypeDesc<CCameraComponent>& desc)
			{
				desc.SetGUID("{A4CB0508-5F07-46B4-B6D4-AB76BFD550F4}"_cry_guid);
				desc.SetEditorCategory("Cameras");
				desc.SetLabel("Camera");
				desc.SetDescription("Represents a camera that can be activated to render to screen");
				desc.SetIcon("icons:General/Camera.ico");
				desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach, IEntityComponent::EFlags::ClientOnly });

				desc.AddMember(&CCameraComponent::m_bActivateOnCreate, 'actv', "Active", "Active", "Whether or not this camera should be activated on component creation", true);
				desc.AddMember(&CCameraComponent::m_nearPlane, 'near', "NearPlane", "Near Plane", nullptr, 0.25f);
				desc.AddMember(&CCameraComponent::m_fieldOfView, 'fov', "FieldOfView", "Field of View", nullptr, 75.0_degrees);
			}

			virtual void Activate()
			{
				m_pEntity->UpdateComponentEventMask(this);

				if (IHmdDevice* pDevice = gEnv->pSystem->GetHmdManager()->GetHmdDevice())
				{
					pDevice->SetAsynCameraCallback(this);
				}

				m_pAudioListener->SetActive(true);

				m_pCameraManager->SwitchCameraToActive(this);
			}

			bool IsActive() const
			{
				return m_pCameraManager->IsThisCameraActive(this);
			}

			void DisableAudioListener()
			{
				m_pAudioListener->SetActive(false);
			}

			virtual void EnableAutomaticActivation(bool bActivate) { m_bActivateOnCreate = bActivate; }
			bool IsAutomaticallyActivated() const { return m_bActivateOnCreate; }

			virtual void SetNearPlane(float nearPlane) { m_nearPlane = nearPlane; }
			float GetNearPlane() const { return m_nearPlane; }

			virtual void SetFieldOfView(CryTransform::CAngle angle) { m_fieldOfView = angle; }
			CryTransform::CAngle GetFieldOfView() const { return m_fieldOfView; }

			virtual CCamera& GetCamera() { return m_camera; }
			const CCamera& GetCamera() const { return m_camera; }

		protected:
			bool m_bActivateOnCreate = true;
			Schematyc::Range<0, 32768> m_nearPlane = 0.25f;
			CryTransform::CClampedAngle<20, 360> m_fieldOfView = 75.0_degrees;

			ICameraManager* m_pCameraManager = nullptr;

			CCamera m_camera;
			Cry::Audio::DefaultComponents::CListenerComponent* m_pAudioListener = nullptr;
		};
	}
}