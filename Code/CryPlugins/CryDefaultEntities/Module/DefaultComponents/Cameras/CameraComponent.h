#pragma once

#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Reflection/TypeDesc.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>

#include <CrySystem/VR/IHMDDevice.h>
#include <CrySystem/VR/IHMDManager.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		class CCameraComponent
			: public IEntityComponent
			, public IHmdDevice::IAsyncCameraCallback
			, public IEntityEventListener
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

			// IEntityComponent
			virtual void Initialize() final;

			virtual void ProcessEvent(SEntityEvent& event) final;
			virtual uint64 GetEventMask() const final;
			// ~IEntityComponent

			// IAsyncCameraCallback
			virtual bool OnAsyncCameraCallback(const HmdTrackingState& state, IHmdDevice::AsyncCameraContext& context) final;
			// ~IAsyncCameraCallback

			// IEntityEventListener
			virtual void OnEntityEvent(IEntity* pEntity, SEntityEvent& event) final;
			// ~IEntityEventListener

		public:
			virtual ~CCameraComponent();

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

				desc.AddMember(&CCameraComponent::m_bAutomaticAudioListenerPosition, 'audi', "AutoAudioListenerPos", "Automatic Audio Listener", "If true, automatically moves the audio listener with the entity.", true);
			}

			virtual void Activate()
			{
				if (s_pActiveCamera != nullptr)
				{
					gEnv->pEntitySystem->GetAreaManager()->ExitAllAreas(s_pActiveCamera->m_pAudioListener->GetId());
					s_pActiveCamera->m_pAudioListener->SetFlagsExtended(s_pActiveCamera->m_pAudioListener->GetFlagsExtended() & ~ENTITY_FLAG_EXTENDED_AUDIO_LISTENER);

					m_pEntity->UpdateComponentEventMask(s_pActiveCamera);
				}

				s_pActiveCamera = this;
				m_pEntity->UpdateComponentEventMask(this);

				if (IHmdDevice* pDevice = gEnv->pSystem->GetHmdManager()->GetHmdDevice())
				{
					pDevice->SetAsynCameraCallback(this);
				}

				if (m_pAudioListener == nullptr)
				{
					// Spawn the audio listener
					SEntitySpawnParams entitySpawnParams;
					entitySpawnParams.sName = "AudioListener";
					entitySpawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();

					// We don't want the audio listener to serialize as the entity gets completely removed and recreated during save/load!
					// NOTE: If we set ENTITY_FLAG_NO_SAVE *after* we spawn the entity, it will make it to m_dynamicEntities in GameSerialize.cpp
					// (via CGameSerialize::OnSpawn) and GameSerialize will attempt to serialize it despite the flag with current (5.2.2) implementation
					entitySpawnParams.nFlags = ENTITY_FLAG_TRIGGER_AREAS | ENTITY_FLAG_NO_SAVE;
					entitySpawnParams.nFlagsExtended = ENTITY_FLAG_EXTENDED_AUDIO_LISTENER;

					m_pAudioListener = gEnv->pEntitySystem->SpawnEntity(entitySpawnParams);
					if (m_pAudioListener != nullptr)
					{
						gEnv->pEntitySystem->AddEntityEventListener(m_pAudioListener->GetId(), ENTITY_EVENT_DONE, this);
						CryFixedStringT<64> sTemp;
						sTemp.Format("AudioListener(%d)", static_cast<int>(m_pAudioListener->GetId()));
						m_pAudioListener->SetName(sTemp.c_str());

						IEntityAudioComponent* pIEntityAudioComponent = m_pAudioListener->GetOrCreateComponent<IEntityAudioComponent>();
						CRY_ASSERT(pIEntityAudioComponent);
					}
					else
					{
						CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "<Audio>: Audio listener creation failed in CCameraComponent::CreateAudioListener!");
					}
				}
				else
				{
					m_pAudioListener->SetFlagsExtended(m_pAudioListener->GetFlagsExtended() | ENTITY_FLAG_EXTENDED_AUDIO_LISTENER);
					m_pAudioListener->InvalidateTM(ENTITY_XFORM_POS);
				}
			}

			virtual void OverrideAudioListenerTransform(const CryTransform::CTransform& transform)
			{
				m_pAudioListener->SetWorldTM(transform.ToMatrix34());

				m_bAutomaticAudioListenerPosition = false;
			}

			bool IsActive() const { return s_pActiveCamera == this; }

			virtual void EnableAutomaticActivation(bool bActivate) { m_bActivateOnCreate = bActivate; }
			bool IsAutomaticallyActivated() const { return m_bActivateOnCreate; }

			virtual void SetNearPlane(float nearPlane) { m_nearPlane = nearPlane; }
			float GetNearPlane() const { return m_nearPlane; }

			virtual void SetFieldOfView(CryTransform::CAngle angle) { m_fieldOfView = angle; }
			CryTransform::CAngle GetFieldOfView() const { return m_fieldOfView; }

			virtual void EnableAutomaticAudioListener(bool bEnable) { m_bAutomaticAudioListenerPosition = bEnable; }
			bool HasAutomaticAudioListener() const { return m_bAutomaticAudioListenerPosition; }

			virtual CCamera& GetCamera() { return m_camera; }
			const CCamera& GetCamera() const { return m_camera; }

		protected:
			bool m_bActivateOnCreate = true;
			Schematyc::Range<0, 32768> m_nearPlane = 0.25f;
			CryTransform::CClampedAngle<20, 360> m_fieldOfView = 75.0_degrees;

			bool m_bAutomaticAudioListenerPosition = true;

			CCamera m_camera;
			IEntity* m_pAudioListener = nullptr;

			static CCameraComponent* s_pActiveCamera;
		};
	}
}