#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
namespace DefaultComponents
{
class CStaticMeshComponent
	: public IEntityComponent
{
protected:
	friend CPlugin_CryDefaultEntities;
	static void Register(Schematyc::CEnvRegistrationScope& componentScope);

	// IEntityComponent
	virtual void   Initialize() final;

	virtual void   ProcessEvent(SEntityEvent& event) final;
	virtual uint64 GetEventMask() const final;
	// ~IEntityComponent

public:
	enum class EType : uint32
	{
		None              = 0,
		Render            = 1 << 0,
		Collider          = 1 << 1,
		RenderAndCollider = Render | Collider,
	};

	CStaticMeshComponent() {}
	virtual ~CStaticMeshComponent() {}

	static void     ReflectType(Schematyc::CTypeDesc<CStaticMeshComponent>& desc);

	static CryGUID& IID()
	{
		static CryGUID id = "{6DDD0033-6AAA-4B71-B8EA-108258205E29}"_cry_guid;
		return id;
	}

	struct SPhysics
	{
		inline bool operator==(const SPhysics& rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

		Schematyc::Range<0, 100000> m_mass = 10.f;
		Schematyc::Range<0, 100000> m_density = 0.f;

		// Used to reset mass or density to -1 in the UI
#ifndef RELEASE
		float m_prevMass = 0.f;
		float m_prevDensity = 0.f;
#endif
	};

	virtual void            SetType(EType type) { m_type = type; }
	EType           GetType() const     { return m_type; }

	virtual void    SetFilePath(const char* szPath);
	const char*     GetFilePath() const          { return m_filePath.value.c_str(); }

	virtual SPhysics&       GetPhysicsParameters()       { return m_physics; }
	const SPhysics& GetPhysicsParameters() const { return m_physics; }

	virtual void LoadFromDisk();
	virtual void ResetObject();

protected:
	Schematyc::GeomFileName m_filePath = "%ENGINE%/EngineAssets/Objects/Default.cgf";
	SPhysics                m_physics;

	EType                   m_type = EType::RenderAndCollider;

	IStatObj*               m_pCachedStatObj = nullptr;
};
}
}
