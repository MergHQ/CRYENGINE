#pragma once

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		class CAreaComponent
			: public IEntityComponent
#ifndef RELEASE
			, public IEntityComponentPreviewer
#endif
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

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

		public:
			enum class EType
			{
				Box,
				Sphere,
				Cylinder,
				Capsule
			};

			struct SShapeParameters
			{
				SShapeParameters(CAreaComponent* pAreaComponent) : pComponent(pAreaComponent) {}
				inline bool operator==(const SShapeParameters &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				void Serialize(Serialization::IArchive& archive);

				CAreaComponent* pComponent;
				Vec3 size = Vec3(1, 1, 1);
			};

			struct SBuoyancyParameters
			{
				enum class EType
				{
					None = -1,
					Air = pe_params_buoyancy::eAir,
					Water = pe_params_buoyancy::eWater
				};

				inline bool operator==(const SBuoyancyParameters &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				EType medium = EType::Air;
				Vec3 flow = Vec3(0, 5, 0);
				Schematyc::PositiveFloat density = 1.f;
				Schematyc::PositiveFloat resistance = 1.f;
			};

			static void ReflectType(Schematyc::CTypeDesc<CAreaComponent>& desc);

			CAreaComponent();
			virtual ~CAreaComponent();

			virtual EType GetType() const { return m_type; }

			static CryGUID& IID()
			{
				static CryGUID id = "{EC7F145B-D48F-4863-B9C2-3D3E2C8DCC61}"_cry_guid;
				return id;
			}

		protected:
			void Physicalize();

		protected:
			EType m_type;
			SShapeParameters m_shapeParameters;

			bool m_bCustomGravity = false;
			Vec3 m_gravity = ZERO;

			SBuoyancyParameters m_buoyancyParameters;
		};
	}
}