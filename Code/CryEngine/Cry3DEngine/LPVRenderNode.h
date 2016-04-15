// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id: LPVRenderNode.h,v 1.0 2008/05/19 12:14:13 AntonKaplanyan Exp wwwrun $
   $DateTime$
   Description:  Routine for rendering and managing of light propagation volumes
   -------------------------------------------------------------------------
   History:
   - 12:6:2008   12:14 : Created by Anton Kaplanyan
*************************************************************************/

#ifndef _LPV_RENDERNODE_
#define _LPV_RENDERNODE_

#pragma once

class CRELightPropagationVolume;

class CLPVRenderNode : public ILPVRenderNode, public Cry3DEngineBase
{
	friend class CLPVCascade;
protected:
	struct SIVSettings
	{
		Matrix34 m_mat;
		float    m_fDensity;            // volume density
	};
public:
	CLPVRenderNode();

	// implements IRenderNode
	virtual void             SetMatrix(const Matrix34& mat);
	virtual void             GetMatrix(Matrix34& mxGrid) const;

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
	virtual const AABB       GetBBox() const                   { return m_WSBBox; }
	virtual void             SetBBox(const AABB& WSBBox)       {}
	virtual void             FillBBox(AABB& aabb);
	virtual void             OffsetPosition(const Vec3& delta) {}

	// implements ILPVRenderNode
	virtual bool  TryInsertLight(const CDLight& light);
	virtual void  SetDensity(const float fDensity);
	virtual float GetDensity() const { return m_settings.m_fDensity; }
	virtual void  UpdateMetrics(const Matrix34& mx, const bool recursive = false); // update grid metrics(e.g. after expansion)
	virtual bool  AutoFit(const DynArray<CDLight>& lightsToFit);                   // tries to auto-fit grid's parameters
	virtual void  EnableSpecular(const bool bEnabled);
	virtual bool  IsSpecularEnabled() const;

	// tries to add a light source into the existing or new volumetric grid
	static bool TryInsertLightIntoVolumes(const CDLight& light);

private:
	typedef std::set<CLPVRenderNode*> LPVSet;

private:
	static void RegisterLPV(CLPVRenderNode* p);
	static void UnregisterLPV(CLPVRenderNode* p);

	bool        ValidateConditions(const SIVSettings& settings);

private:
	~CLPVRenderNode();

private:
	static LPVSet              ms_volumes;
public:
	SIVSettings                m_settings;

	AABB                       m_WSBBox;

	Vec3                       m_pos;
	Vec3                       m_size;

	Matrix34                   m_matInv;

	_smart_ptr<IMaterial>      m_pMaterial;

	int                        m_nGridWidth;      // grid dimensions in texels
	int                        m_nGridHeight;     // grid dimensions in texels
	int                        m_nGridDepth;      // grid dimensions in texels
	int                        m_nNumIterations;  // number of iterations to propagate
	Vec4                       m_gridDimensions;  // grid size
	Vec3                       m_gridCellSize;    // the distance between two neighbor cells
	float                      m_maxGridCellSize; // the max of m_gridCellSize components
	float                      m_bIsValid;        // the validation flag

	CRELightPropagationVolume* m_pRE;
};

#endif // #ifndef _LPV_RENDERNODE_
