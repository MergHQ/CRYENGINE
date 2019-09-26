#pragma once

#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Reflection/TypeDesc.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		class CBreakableJointComponent
			: public IEntityComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

			// IEntityComponent
			virtual void Initialize() final;

			virtual void ProcessEvent(const SEntityEvent& event) final;
			virtual Cry::Entity::EventFlags GetEventMask() const final { return ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED | ENTITY_EVENT_PHYSICAL_TYPE_CHANGED | ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED; }
			// ~IEntityComponent

		public:
			virtual ~CBreakableJointComponent() {}

			static void ReflectType(Schematyc::CTypeDesc<CBreakableJointComponent>& desc);

			struct SDynConstraint 
			{
				inline bool operator==(const SDynConstraint& rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }
				CryTransform::CAngle     m_minAngle = -90_degrees;
				CryTransform::CAngle     m_maxAngle = 90_degrees;
				Schematyc::PositiveFloat m_maxForce = 0.0f;
				bool                     m_noColl = false;
				Schematyc::PositiveFloat m_damping = 0.0f;
			};

			struct SBrokenSignal
			{
				int  idJoint;
				Vec3 point, normal;

				SBrokenSignal() {}
				SBrokenSignal(int id, const Vec3 &pt, const Vec3 &n) : idJoint(id), point(pt), normal(n) {}
			};

		protected:
			void Physicalize();

		protected:
			Schematyc::PositiveFloat m_maxForcePush = 100000.0f, m_maxForcePull = 100000.0f, m_maxForceShift = 100000.0f;
			Schematyc::PositiveFloat m_maxTorqueBend = 100000.0f, m_maxTorqueTwist = 100000.0f;
			bool                     m_defDmgAccum = false;
			Schematyc::Range<0,1>    m_damageAccum = 0.1f, m_damageAccumThresh = 0.0f;
			bool                     m_breakable = true;
			bool                     m_directBreaksOnly = false;
			Schematyc::PositiveFloat m_szSensor = 0.05f;

			SDynConstraint m_constr;
		};

	}
}