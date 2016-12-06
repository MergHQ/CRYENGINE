// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef DYNAMICRESPONSESYSTEM_PROXY_H_
#define DYNAMICRESPONSESYSTEM_PROXY_H_

#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>

//////////////////////////////////////////////////////////////////////////
// Description:
//    Implements dynamic response proxy class for entity.
//////////////////////////////////////////////////////////////////////////
class CEntityComponentDynamicResponse final : public IEntityDynamicResponseComponent
{
	CRY_ENTITY_COMPONENT_CLASS(CEntityComponentDynamicResponse,IEntityDynamicResponseComponent,"CEntityComponentDynamicResponse",0x891F8E50BAF84E95,0xB5F94F7BC07EB663);

	CEntityComponentDynamicResponse();
	virtual ~CEntityComponentDynamicResponse();

public:
	//IEntityComponent.h override
	virtual void Initialize() override;

	//////////////////////////////////////////////////////////////////////////
	// IEntityComponent interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void         ProcessEvent(SEntityEvent& event) override;
	virtual uint64       GetEventMask() const override;
	virtual EEntityProxy GetProxyType() const override { return ENTITY_PROXY_DYNAMICRESPONSE; }
	virtual bool         NeedGameSerialize() override;
	virtual void         GameSerialize(TSerialize ser) override;
	virtual void         GetMemoryUsage(ICrySizer* pSizer) const override
	{
		pSizer->AddObject(this, sizeof(*this));
	}
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityDynamicResponseComponent interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual DRS::IVariableCollection* GetLocalVariableCollection() const override;
	virtual DRS::IResponseActor*      GetResponseActor() const override;
	//////////////////////////////////////////////////////////////////////////

private:
	DRS::IResponseActor* m_pResponseActor;
};

#endif // DYNAMICRESPONSESYSTEM_PROXY_H_
