#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CryPhysics/IPhysics.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		class CWaterVolumeComponent
			: public IEntityComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

			// IEntityComponent
			virtual void Initialize() final {}

			virtual void ProcessEvent(const SEntityEvent& event) final;
			virtual Cry::Entity::EventFlags GetEventMask() const final;

			virtual NetworkAspectType GetNetSerializeAspectMask() const override { return eEA_GameClientDynamic; };
			// ~IEntityComponent

		public:
			static void ReflectType(Schematyc::CTypeDesc<CWaterVolumeComponent>& desc)
			{
				desc.SetGUID("{1BCEB9A2-79B0-495C-998A-1574D9C1E0D1}"_cry_guid);
				desc.SetEditorCategory("Physics");
				desc.SetLabel("WaterVolume");
				desc.SetDescription("The components attaches itself to a physicalized water volume on game start and allows changing some of its parameters dynamically");
				desc.SetIcon("icons:ObjectTypes/object.ico");
				desc.SetComponentFlags({ IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach, IEntityComponent::EFlags::Singleton });

				desc.AddMember(&CWaterVolumeComponent::m_distAttach, 'atds', "AttachDist", "Attachment Distance", "Distance around the component's origin that gets sampled for water volume attachment", 0.1f);
			}

			virtual void SetDensity(float dens)
			{
				pe_params_buoyancy pb;
				pb.waterDensity = dens;
				if (m_volume && m_volume->GetPhysics())
					m_volume->GetPhysics()->SetParams(&pb);
			}
			virtual void SetResistance(float res)
			{
				pe_params_buoyancy pb;
				pb.waterResistance = res;
				if (m_volume && m_volume->GetPhysics())
					m_volume->GetPhysics()->SetParams(&pb);
			}
			virtual void SetFlow(const Vec3& flow)
			{
				pe_params_buoyancy pb;
				pb.waterFlow = flow;
				if (m_volume && m_volume->GetPhysics())
					m_volume->GetPhysics()->SetParams(&pb);
			}
			virtual void SetFixedVolume(float vol)
			{
				pe_params_area pa;
				pa.volume = vol;
				if (m_volume && m_volume->GetPhysics())
					m_volume->GetPhysics()->SetParams(&pa);
			}

			CWaterVolumeComponent() {}
			virtual ~CWaterVolumeComponent() {}

		protected:
			void Attach();

			float m_distAttach = 0.1f;

			IWaterVolumeRenderNode* m_volume = nullptr; 
		};
	}
}