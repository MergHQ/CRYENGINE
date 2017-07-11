#pragma once

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		class CThrusterComponent
			: public IEntityComponent
#ifndef RELEASE
			, public IEntityComponentPreviewer
#endif
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

		public:
			static void ReflectType(Schematyc::CTypeDesc<CThrusterComponent>& desc);

			CThrusterComponent() {}
			virtual ~CThrusterComponent() {}

			// IEntityComponent
			virtual void Initialize() final;

			virtual void ProcessEvent(SEntityEvent& event) final;
			virtual uint64 GetEventMask() const final;

#ifndef RELEASE
			virtual IEntityComponentPreviewer* GetPreviewer() final { return this; }
#endif
			// ~IEntityComponent

#ifndef RELEASE
			// IEntityComponentPreviewer
			virtual void SerializeProperties(Serialization::IArchive& archive) final {}
			virtual void Render(const IEntity& entity, const IEntityComponent& component, SEntityPreviewContext &context) const final;
			// ~IEntityComponentPreviewer
#endif

			static CryGUID& IID()
			{
				static CryGUID id = "{B3222B05-BE90-4E19-B0A8-C0250F62AC98}"_cry_guid;
				return id;
			}

			virtual void EnableConstantThrust(bool bEnable);
			virtual bool IsConstantThrustEnabled() const { return m_bConstantThrustActive; }

			virtual void ApplySingleThrust(float thrust);
			
		protected:
			bool m_bEnableConstantThrustByDefault = true;
			float m_constantThrust = 1;

			bool m_bConstantThrustActive = true;
		};
	}
}