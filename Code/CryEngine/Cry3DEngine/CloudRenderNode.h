// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   CloudRenderNode.h
//  Version:     v1.00
//  Created:     15/2/2005 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CloudRenderNode_h__
#define __CloudRenderNode_h__
#pragma once

struct SCloudDescription;

//////////////////////////////////////////////////////////////////////////
// RenderNode for rendering single cloud object.
//////////////////////////////////////////////////////////////////////////
class CCloudRenderNode : public ICloudRenderNode, public Cry3DEngineBase
{
public:

	CCloudRenderNode();

	//////////////////////////////////////////////////////////////////////////
	// Implements ICloudRenderNode
	//////////////////////////////////////////////////////////////////////////
	virtual bool LoadCloud(const char* sCloudFilename);
	virtual bool LoadCloudFromXml(XmlNodeRef cloudNode);
	virtual void SetMovementProperties(const SCloudMovementProperties& properties);

	//////////////////////////////////////////////////////////////////////////
	// Implements IRenderNode
	//////////////////////////////////////////////////////////////////////////
	virtual void             GetLocalBounds(AABB& bbox) { bbox = m_bounds; };
	virtual void             SetMatrix(const Matrix34& mat);

	virtual EERType          GetRenderNodeType();
	virtual const char*      GetEntityClassName() const { return "Cloud"; }
	virtual const char*      GetName() const            { return "Cloud"; }
	virtual Vec3             GetPos(bool bWorldOnly = true) const;
	virtual void             Render(const SRendParams& rParam, const SRenderingPassInfo& passInfo);
	virtual IPhysicalEntity* GetPhysics() const           { return 0; }
	virtual void             SetPhysics(IPhysicalEntity*) {}
	virtual void             SetMaterial(IMaterial* pMat) { m_pMaterial = pMat; }
	virtual IMaterial*       GetMaterial(Vec3* pHitPos = NULL) const;
	virtual IMaterial*       GetMaterialOverride()        { return m_pMaterial; }
	virtual float            GetMaxViewDist();
	virtual const AABB       GetBBox() const              { return m_WSBBox; }
	virtual void             SetBBox(const AABB& WSBBox)  { m_WSBBox = WSBBox; }
	virtual void             FillBBox(AABB& aabb);
	virtual void             OffsetPosition(const Vec3& delta);
	//////////////////////////////////////////////////////////////////////////

	bool CheckIntersection(const Vec3& p1, const Vec3& p2);
	void MoveCloud();

	void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
private:
	void         SetCloudDesc(SCloudDescription* pCloud);
	~CCloudRenderNode();
	virtual void SetMatrixInternal(const Matrix34& mat, bool updateOrigin);

private:
	Vec3                          m_pos;
	float                         m_fScale;
	_smart_ptr<IMaterial>         m_pMaterial;
	_smart_ptr<SCloudDescription> m_pCloudDesc;
	Matrix34                      m_matrix;
	Matrix34                      m_offsetedMatrix;
	Vec3                          m_vOffset;
	AABB                          m_bounds;

	CREBaseCloud*                 m_pCloudRenderElement;
	CREImposter*                  m_pREImposter;
	float                         m_alpha;

	Vec3                          m_origin;
	SCloudMovementProperties      m_moveProps;

	AABB                          m_WSBBox;
};

#endif // __CloudRenderNode_h__
