// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __ENTITYATTRIBUTESPROXY_H__
#define __ENTITYATTRIBUTESPROXY_H__

#pragma once

#include "EntitySystem.h"
#include <CryEntitySystem/IEntityAttributesProxy.h>
#include <CryEntitySystem/IEntityClass.h>
#include <CryEntitySystem/IEntityComponent.h>
#include <CryNetwork/ISerialize.h>

//////////////////////////////////////////////////////////////////////////
// Description:
//    Proxy for storage of entity attributes.
//////////////////////////////////////////////////////////////////////////
class CEntityComponentAttributes : public IEntityAttributesComponent
{
	CRY_ENTITY_COMPONENT_CLASS(CEntityComponentAttributes,IEntityAttributesComponent,"CEntityComponentAttributes",0x05A253F3463A44F6,0xB0629551A267FC19);

public:

	// IEntityComponent.h
	virtual void ProcessEvent(SEntityEvent& event) override;
	virtual void Initialize(SComponentInitializer const& inititializer) override;
	virtual uint64 GetEventMask() const final;;
	// ~IEntityComponent.h

	// IEntityComponent
	virtual EEntityProxy GetProxyType() const override;
	virtual void         Release() final { delete this; };
	virtual void         SerializeXML(XmlNodeRef& entityNodeXML, bool loading) override;
	virtual void         GameSerialize(TSerialize serialize) override;
	virtual bool         NeedGameSerialize() override;
	virtual void         GetMemoryUsage(ICrySizer* pSizer) const override;
	// ~IEntityComponent

	// IEntityAttributesComponent
	virtual void                         SetAttributes(const TEntityAttributeArray& attributes) override;
	virtual TEntityAttributeArray&       GetAttributes() override;
	virtual const TEntityAttributeArray& GetAttributes() const override;
	// ~IEntityAttributesComponent

private:

	TEntityAttributeArray m_attributes;
};

inline CEntityComponentAttributes::CEntityComponentAttributes() {}
inline CEntityComponentAttributes::~CEntityComponentAttributes() {}

DECLARE_SHARED_POINTERS(CEntityComponentAttributes)

#endif //__ENTITYATTRIBUTESPROXY_H__
