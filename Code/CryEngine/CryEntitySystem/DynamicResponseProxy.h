// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef DYNAMICRESPONSESYSTEM_PROXY_H_
#define DYNAMICRESPONSESYSTEM_PROXY_H_

#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>

//////////////////////////////////////////////////////////////////////////
// Description:
//    Implements dynamic response proxy class for entity.
//////////////////////////////////////////////////////////////////////////
class CDynamicResponseProxy final : public IEntityDynamicResponseProxy
{
public:
	CDynamicResponseProxy();
	virtual ~CDynamicResponseProxy();

	//IComponent override
	virtual void Initialize(SComponentInitializer const& init) override;

	//////////////////////////////////////////////////////////////////////////
	// IEntityProxy interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void         ProcessEvent(SEntityEvent& event) override;
	virtual EEntityProxy GetType() override                                          { return ENTITY_PROXY_DYNAMICRESPONSE; }
	virtual void         Release() override;
	virtual void         Done() override                                             {}
	virtual void         Update(SEntityUpdateContext& ctx) override;
	virtual bool         Init(IEntity* pEntity, SEntitySpawnParams& params) override { return true; }
	virtual void         Reload(IEntity* pEntity, SEntitySpawnParams& params) override;
	virtual void         SerializeXML(XmlNodeRef& entityNode, bool bLoading) override;
	virtual bool         NeedSerialize() override;
	virtual void         Serialize(TSerialize ser) override;
	virtual bool         GetSignature(TSerialize signature) override;
	virtual void         GetMemoryUsage(ICrySizer* pSizer) const override
	{
		pSizer->AddObject(this, sizeof(*this));
	}
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityDynamicResponseProxy interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual DRS::IVariableCollection* GetLocalVariableCollection() const override;
	virtual DRS::IResponseActor*      GetResponseActor() const override;
	//////////////////////////////////////////////////////////////////////////

private:
	DRS::IResponseActor* m_pResponseActor;
};

#endif // DYNAMICRESPONSESYSTEM_PROXY_H_
