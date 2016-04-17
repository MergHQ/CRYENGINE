// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   CameraProxy.h
//  Version:     v1.00
//  Created:     5/12/2005 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CameraProxy_h__
#define __CameraProxy_h__
#pragma once

#include "Entity.h"
#include "EntitySystem.h"

struct SProximityElement;

//////////////////////////////////////////////////////////////////////////
// Description:
//    Handles sounds in the entity.
//////////////////////////////////////////////////////////////////////////
class CCameraProxy : public IEntityCameraProxy
{
public:
	CCameraProxy();
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
	virtual EEntityProxy GetType()                         { return ENTITY_PROXY_CAMERA; }
	virtual void         Release();
	virtual void         Done()                            {};
	virtual void         Update(SEntityUpdateContext& ctx) {};
	virtual bool         Init(IEntity* pEntity, SEntitySpawnParams& params);
	virtual void         Reload(IEntity* pEntity, SEntitySpawnParams& params);
	virtual void         SerializeXML(XmlNodeRef& entityNode, bool bLoading) {};
	virtual void         Serialize(TSerialize ser);
	virtual bool         NeedSerialize()                                     { return false; };
	virtual bool         GetSignature(TSerialize signature);
	//////////////////////////////////////////////////////////////////////////

	virtual void     SetCamera(CCamera& cam);
	virtual CCamera& GetCamera() { return m_camera; };
	//////////////////////////////////////////////////////////////////////////

	void         UpdateMaterialCamera();

	virtual void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
private:
	//////////////////////////////////////////////////////////////////////////
	// Private member variables.
	//////////////////////////////////////////////////////////////////////////
	// Host entity.
	CEntity* m_pEntity;
	CCamera  m_camera;
};

#endif // __CameraProxy_h__
