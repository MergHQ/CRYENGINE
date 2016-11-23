// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   RopeProxy.h
//  Version:     v1.00
//  Created:     23/10/2006 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __RopeProxy_h__
#define __RopeProxy_h__
#pragma once

// forward declarations.
struct SEntityEvent;
struct IPhysicalEntity;
struct IPhysicalWorld;

//////////////////////////////////////////////////////////////////////////
// Description:
//    Implements rope proxy class for entity.
//////////////////////////////////////////////////////////////////////////
class CEntityComponentRope : public IEntityRopeComponent
{
	CRY_ENTITY_COMPONENT_CLASS(CEntityComponentRope,IEntityRopeComponent,"CEntityComponentRope",0xDFAE2B7E15BB4F3D,0xBD09E0C8E560BF85);

	CEntityComponentRope();
	virtual ~CEntityComponentRope();

public:
	//////////////////////////////////////////////////////////////////////////
	// IEntityComponent interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void Initialize() final;
	virtual void ProcessEvent(SEntityEvent& event) final;
	virtual uint64 GetEventMask() const final;
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityComponent interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual EEntityProxy GetProxyType() const final { return ENTITY_PROXY_ROPE; }
	virtual void         Release() final { delete this; };
	virtual void         Update(SEntityUpdateContext& ctx) final;
	virtual void         SerializeXML(XmlNodeRef& entityNode, bool bLoading) final;
	virtual bool         NeedGameSerialize() final;
	virtual void         GameSerialize(TSerialize ser) final;
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	/// IEntityRopeComponent
	//////////////////////////////////////////////////////////////////////////
	virtual IRopeRenderNode* GetRopeRenderNode() final { return m_pRopeRenderNode; };
	//////////////////////////////////////////////////////////////////////////

	virtual void GetMemoryUsage(ICrySizer* pSizer) const final
	{
		pSizer->AddObject(this, sizeof(*this));
	}
	void PreserveParams();
protected:
	IRopeRenderNode* m_pRopeRenderNode;
	int              m_nSegmentsOrg;
	float            m_texTileVOrg;
};

#endif // __RopeProxy_h__
