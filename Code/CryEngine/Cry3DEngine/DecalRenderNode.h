// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef _DECAL_RENDERNODE_
#define _DECAL_RENDERNODE_

#pragma once

#include "DecalManager.h"

class CDecalRenderNode : public IDecalRenderNode, public Cry3DEngineBase
{
public:
	// implements IDecalRenderNode
	virtual void                    SetDecalProperties(const SDecalProperties& properties);
	virtual const SDecalProperties* GetDecalProperties() const;
	virtual void                    CleanUpOldDecals();

	// implements IRenderNode
	virtual IRenderNode*     Clone() const;
	virtual void             SetMatrix(const Matrix34& mat);
	virtual const Matrix34&  GetMatrix() { return m_Matrix; }

	virtual EERType          GetRenderNodeType();
	virtual const char*      GetEntityClassName() const;
	virtual const char*      GetName() const;
	virtual Vec3             GetPos(bool bWorldOnly = true) const;
	virtual void             Render(const SRendParams& rParam, const SRenderingPassInfo& passInfo);
	virtual IPhysicalEntity* GetPhysics() const;
	virtual void             SetPhysics(IPhysicalEntity*);
	virtual void             SetMaterial(IMaterial* pMat);
	virtual IMaterial*       GetMaterial(Vec3* pHitPos = 0) const;
	virtual IMaterial*       GetMaterialOverride() { return m_pMaterial; }
	virtual float            GetMaxViewDist();
	virtual void             Precache();
	virtual void             GetMemoryUsage(ICrySizer* pSizer) const;

	virtual const AABB       GetBBox() const             { return m_WSBBox; }
	virtual void             SetBBox(const AABB& WSBBox) { m_WSBBox = WSBBox; }
	virtual void             FillBBox(AABB& aabb);
	virtual void             OffsetPosition(const Vec3& delta);

	virtual uint8            GetSortPriority()           { return m_decalProperties.m_sortPrio; }

	virtual void             SetLayerId(uint16 nLayerId) { m_nLayerId = nLayerId; Get3DEngine()->C3DEngine::UpdateObjectsLayerAABB(this); }
	virtual uint16           GetLayerId()                { return m_nLayerId; }
	static void              ResetDecalUpdatesCounter()  { CDecalRenderNode::m_nFillBigDecalIndicesCounter = 0; }

	// SetMatrix only supports changing position, this will do the full transform
	void SetMatrixFull(const Matrix34& mat);
public:
	CDecalRenderNode();
	void RequestUpdate() { m_updateRequested = true; DeleteDecals(); }
	void DeleteDecals();

private:
	~CDecalRenderNode();
	void CreateDecals();
	void ProcessUpdateRequest();
	void UpdateAABBFromRenderMeshes();
	bool CheckForceDeferred();

	void CreatePlanarDecal();
	void CreateDecalOnStaticObjects();
	void CreateDecalOnTerrain();

private:
	Vec3                  m_pos;
	AABB                  m_localBounds;
	_smart_ptr<IMaterial> m_pMaterial;
	bool                  m_updateRequested;
	SDecalProperties      m_decalProperties;
	std::vector<CDecal*>  m_decals;
	AABB                  m_WSBBox;
	Matrix34              m_Matrix;
	uint32                m_nLastRenderedFrameId;
	uint16                m_nLayerId;
	IPhysicalEntity*      m_physEnt;

public:
	static int m_nFillBigDecalIndicesCounter;
};

#endif // #ifndef _DECAL_RENDERNODE_
