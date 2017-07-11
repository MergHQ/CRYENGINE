#pragma once

#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/Reflection/TypeDesc.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		class CChildEntityComponent
			: public IEntityComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

			// IEntityComponent
			virtual void Initialize() final;

			virtual void ProcessEvent(SEntityEvent& event) final;
			virtual uint64 GetEventMask() const final;
			// ~IEntityComponent

		public:
			CChildEntityComponent() = default;
			virtual ~CChildEntityComponent();

			static void ReflectType(Schematyc::CTypeDesc<CChildEntityComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "{BCB58434-4EA2-4C61-A8F7-A264A6B87636}"_cry_guid;
				return id;
			}

			Schematyc::ExplicitEntityId GetChildEntityId() const { return Schematyc::ExplicitEntityId(m_childEntityId); }

		protected:
			void SpawnEntity();
			void RemoveEntity();
			void AddIgnoreConstraint();
			void RemoveIgnoreConstraint();

		protected:
			Schematyc::EntityClassName m_className;
			bool m_bLinkTransformation = false;
			bool m_bIgnoreContactsWithChild = true;
			
			EntityId m_childEntityId = INVALID_ENTITYID;
			int m_constraintId = -1;
			int m_childConstraintId = -1;
		};
	}
}