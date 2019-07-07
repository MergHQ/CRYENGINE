// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "Cry3DEngineBase.h"

class CStatObjFoliage : public IFoliage, public Cry3DEngineBase
{
public:
	CStatObjFoliage();
	~CStatObjFoliage();

	virtual void             AddRef() { m_nRefCount++; }
	virtual void             Release() { if (--m_nRefCount <= 0) m_bDelete = 2; }

	virtual int              Serialize(TSerialize ser);
	virtual void             SetFlags(uint flags);
	virtual uint             GetFlags() { return m_flags; }
	virtual IRenderNode*     GetIRenderNode() { return m_pVegInst; }
	virtual int              GetBranchCount() { return m_nRopes; }
	virtual IPhysicalEntity* GetBranchPhysics(int iBranch) { return (unsigned int)iBranch < (unsigned int)m_nRopes ? m_pRopes[iBranch] : 0; }

	virtual SSkinningData*   GetSkinningData(const Matrix34& RenderMat34, const SRenderingPassInfo& passInfo);

	uint32                   ComputeSkinningTransformationsCount();
	void                     ComputeSkinningTransformations(uint32 nList);

	void                     OnHit(struct EventPhysCollision* pHit);
	void                     Update(float dt, const CCamera& rCamera);
	void                     BreakBranch(int idx);

	CStatObjFoliage*  m_next;
	CStatObjFoliage*  m_prev;
	int               m_nRefCount;
	uint              m_flags;
	CStatObj*         m_pStatObj;
	IPhysicalEntity** m_pRopes;
	float*            m_pRopesActiveTime;
	IPhysicalEntity*  m_pTrunk;
	int16             m_nRopes;
	int16             m_bEnabled;
	float             m_timeIdle, m_lifeTime;
	IFoliage**        m_ppThis;
	QuatTS*           m_pSkinningTransformations[2];
	int               m_iActivationSource;
	int               m_bGeomRemoved;
	IRenderNode*      m_pVegInst;
	CRenderObject*    m_pRenderObject;
	float             m_timeInvisible;
	float             m_minEnergy;
	float             m_stiffness;
	int               m_bDelete;
	// history for skinning data, needed for motion blur
	struct { SSkinningData* pSkinningData; int nFrameID; } arrSkinningRendererData[3]; // tripple buffered for motion blur
};
