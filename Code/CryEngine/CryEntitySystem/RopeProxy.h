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
class CRopeProxy : public IEntityRopeProxy
{
public:
	CRopeProxy();
	~CRopeProxy();
	CEntity* GetEntity() const { return m_pEntity; };

	//////////////////////////////////////////////////////////////////////////
	// IEntityProxy interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void Initialize(const SComponentInitializer& init);
	virtual void ProcessEvent(SEntityEvent& event);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityProxy interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual EEntityProxy GetType()                                          { return ENTITY_PROXY_ROPE; }
	virtual void         Release();
	virtual void         Done()                                             {};
	virtual void         Update(SEntityUpdateContext& ctx);
	virtual bool         Init(IEntity* pEntity, SEntitySpawnParams& params) { return true; }
	virtual void         Reload(IEntity* pEntity, SEntitySpawnParams& params);
	virtual void         SerializeXML(XmlNodeRef& entityNode, bool bLoading);
	virtual bool         NeedSerialize();
	virtual void         Serialize(TSerialize ser);
	virtual bool         GetSignature(TSerialize signature);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	/// IEntityRopeProxy
	//////////////////////////////////////////////////////////////////////////
	virtual IRopeRenderNode* GetRopeRenderNode() { return m_pRopeRenderNode; };
	//////////////////////////////////////////////////////////////////////////

	virtual void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
	void PreserveParams();
protected:
	CEntity*         m_pEntity;

	IRopeRenderNode* m_pRopeRenderNode;
	int              m_nSegmentsOrg;
	float            m_texTileVOrg;
};

#endif // __RopeProxy_h__
