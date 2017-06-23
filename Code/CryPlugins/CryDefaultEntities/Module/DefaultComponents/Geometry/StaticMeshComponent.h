#pragma once

#include "BaseMeshComponent.h"

class CPlugin_CryDefaultEntities;

namespace Cry
{
namespace DefaultComponents
{
class CStaticMeshComponent
	: public CBaseMeshComponent
{
protected:
	friend CPlugin_CryDefaultEntities;
	static void Register(Schematyc::CEnvRegistrationScope& componentScope);

	// IEntityComponent
	virtual void   Initialize() final;

	virtual void   ProcessEvent(SEntityEvent& event) final;
	// ~IEntityComponent

public:
	CStaticMeshComponent() {}
	virtual ~CStaticMeshComponent() {}

	static void     ReflectType(Schematyc::CTypeDesc<CStaticMeshComponent>& desc);

	static CryGUID& IID()
	{
		static CryGUID id = "{6DDD0033-6AAA-4B71-B8EA-108258205E29}"_cry_guid;
		return id;
	}

	virtual void    SetFilePath(const char* szPath);
	const char*     GetFilePath() const { return m_filePath.value.c_str(); }

	virtual void LoadFromDisk();
	virtual void SetObject(IStatObj* pObject, bool bSetDefaultMass = false);
	virtual void ResetObject();

protected:
	Schematyc::GeomFileName m_filePath = "%ENGINE%/EngineAssets/Objects/Default.cgf";
	
	_smart_ptr<IStatObj>    m_pCachedStatObj = nullptr;
};
}
}
