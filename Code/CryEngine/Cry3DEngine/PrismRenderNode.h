// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef _PRISM_RENDERNODE_
#define _PRISM_RENDERNODE_

#pragma once

class CPrismRenderNode : public IPrismRenderNode, public Cry3DEngineBase
{
public:
	CPrismRenderNode();

	// interface IPrismRenderNode --------------------------------------------

	// interface IRenderNode -------------------------------------------------
	virtual void             SetMatrix(const Matrix34& mat);
	virtual EERType          GetRenderNodeType();
	virtual const char*      GetEntityClassName() const { return "PrismObject"; }
	virtual const char*      GetName() const;
	virtual Vec3             GetPos(bool bWorldOnly = true) const;
	virtual void             Render(const SRendParams& rParam, const SRenderingPassInfo& passInfo);
	virtual IPhysicalEntity* GetPhysics() const           { return 0; }
	virtual void             SetPhysics(IPhysicalEntity*) {}
	virtual void             SetMaterial(IMaterial* pMat) { m_pMaterial = pMat; }
	virtual IMaterial*       GetMaterial(Vec3* pHitPos = 0) const;
	virtual IMaterial*       GetMaterialOverride()        { return m_pMaterial; }
	virtual float            GetMaxViewDist();
	virtual void             GetMemoryUsage(ICrySizer* pSizer) const;
	virtual const AABB       GetBBox() const             { return m_WSBBox; }
	virtual void             SetBBox(const AABB& WSBBox) { m_WSBBox = WSBBox; }
	virtual void             FillBBox(AABB& aabb);
	virtual void             OffsetPosition(const Vec3& delta);

private:
	~CPrismRenderNode();

	AABB                  m_WSBBox;
	Matrix34              m_mat;
	_smart_ptr<IMaterial> m_pMaterial;
	CREPrismObject*       m_pRE;
};

#endif // #ifndef _PRISM_RENDERNODE_
