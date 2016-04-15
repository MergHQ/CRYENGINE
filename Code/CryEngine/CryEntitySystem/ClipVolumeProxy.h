// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __CLIPVOLUMEPROXY_H__
#define __CLIPVOLUMEPROXY_H__

#pragma once

#include "EntitySystem.h"
#include <CryEntitySystem/IEntityClass.h>
#include <CryEntitySystem/IEntityProxy.h>
#include <CryNetwork/ISerialize.h>
#include <CryRenderer/IRenderMesh.h>

//////////////////////////////////////////////////////////////////////////
// Description:
//    Proxy for storage of entity attributes.
//////////////////////////////////////////////////////////////////////////
class CClipVolumeProxy : public IClipVolumeProxy
{
public:
	CClipVolumeProxy();

	// IComponent interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void Initialize(const SComponentInitializer& init) override {};
	virtual void ProcessEvent(SEntityEvent& event) override;
	//////////////////////////////////////////////////////////////////////////

	// IEntityProxy
	//////////////////////////////////////////////////////////////////////////
	virtual EEntityProxy GetType() override                             { return ENTITY_PROXY_CLIPVOLUME; }
	virtual void         Release() override;
	virtual void         Done() override                                {};
	virtual void         Update(SEntityUpdateContext& context) override {};
	virtual bool         Init(IEntity* pEntity, SEntitySpawnParams& params) override;
	virtual void         Reload(IEntity* pEntity, SEntitySpawnParams& params) override;
	virtual void         SerializeXML(XmlNodeRef& entityNodeXML, bool loading) override;
	virtual void         Serialize(TSerialize serialize) override {};
	virtual bool         NeedSerialize() override                 { return false; }
	virtual bool         GetSignature(TSerialize signature) override;
	virtual void         GetMemoryUsage(ICrySizer* pSizer) const override;
	//////////////////////////////////////////////////////////////////////////

	virtual void         UpdateRenderMesh(IRenderMesh* pRenderMesh, const DynArray<Vec3>& meshFaces) override;
	virtual IClipVolume* GetClipVolume() const override { return m_pClipVolume; }
	virtual IBSPTree3D*  GetBspTree() const override    { return m_pBspTree; }
	virtual void         SetProperties(bool bIgnoresOutdoorAO) override;

private:
	bool LoadFromFile(const char* szFilePath);

private:
	// Host entity.
	IEntity* m_pEntity;

	// Engine clip volume
	IClipVolume* m_pClipVolume;

	// Render Mesh
	_smart_ptr<IRenderMesh> m_pRenderMesh;

	// BSP tree
	IBSPTree3D* m_pBspTree;

	// In-game stat obj
	string m_GeometryFileName;

	// Clip volume flags
	uint32 m_nFlags;
};

DECLARE_SHARED_POINTERS(CClipVolumeProxy)

#endif //__CLIPVOLUMEPROXY_H__
