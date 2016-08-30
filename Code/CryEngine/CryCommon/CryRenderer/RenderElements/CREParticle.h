// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __CREPARTICLE_H__
#define __CREPARTICLE_H__

#include <CryThreading/IJobManager.h>
#include <CryMemory/MemoryAccess.h>

// forward declarations
class CREParticle;
typedef SVF_P3F_C4B_T4B_N3F2 SVF_Particle;

struct SParticleAxes
{
	Vec3 xAxis;
	Vec3 yAxis;
};

struct SParticleColorST
{
	UCol color;
	UCol st;
};

struct SRenderVertices
{
	FixedDynArray<SVF_Particle>     aVertices;
	FixedDynArray<uint16>           aIndices;
	FixedDynArray<Vec3>             aPositions;
	FixedDynArray<SParticleAxes>    aAxes;
	FixedDynArray<SParticleColorST> aColorSTs;
	float                           fPixels;

	ILINE SRenderVertices()
		: fPixels(0.f) {}
};

struct SCameraInfo
{
	const CCamera* pCamera;
	IVisArea*      pCameraVisArea;
	bool           bCameraUnderwater;
	bool           bRecursivePass;

	SCameraInfo(const SRenderingPassInfo& passInfo) :
		pCamera(&passInfo.GetCamera()),
		pCameraVisArea(gEnv->p3DEngine->GetVisAreaFromPos(passInfo.GetCamera().GetOccPos())),
		bCameraUnderwater(passInfo.IsCameraUnderWater()),
		bRecursivePass(passInfo.IsRecursivePass())
	{}
};

struct IParticleVertexCreator
{
	//! Create the vertices for the particle emitter.
	virtual void ComputeVertices(const SCameraInfo& camInfo, CREParticle* pRE, uint64 uRenderFlags, float fMaxPixels) = 0;

	virtual ~IParticleVertexCreator() {}
};

class CREParticle : public CRendElementBase
{
public:
	static const uint numBuffers = 3;

	enum EParticleObjFlags
	{
		ePOF_HALF_RES              = BIT(0),
		ePOF_VOLUME_FOG            = BIT(1),
		ePOF_USE_VERTEX_PULL_MODEL = BIT(2),
	};

public:
	CREParticle();

	//! Custom copy constructor required to avoid m_Lock copy.
	CREParticle(const CREParticle& in)
		: m_pVertexCreator(in.m_pVertexCreator)
		, m_nThreadId(in.m_nThreadId)
	{
	}

	void Reset(IParticleVertexCreator* pVC, int nThreadId, uint allocId);

	virtual void GetMemoryUsage(ICrySizer* pSizer) const
	{
	}

	//! CRendElement implementation.
	virtual CRendElementBase* mfCopyConstruct()
	{
		return new CREParticle(*this);
	}
	virtual int Size()
	{
		return sizeof(*this);
	}

	virtual void mfPrepare(bool bCheckOverflow);
	virtual bool mfDraw(CShader* ef, SShaderPass* sl);

	// Additional methods.

	//! Interface to alloc render verts and indices from 3DEngine code.
	virtual SRenderVertices* AllocVertices(int nAllocVerts, int nAllocInds);
	virtual SRenderVertices* AllocPullVertices(int nPulledVerts);

	void                     ComputeVertices(SCameraInfo camInfo, uint64 uRenderFlags);

	bool                     AddedToView() const { return m_addedToView != 0; }
	void                     SetAddedToView() { m_addedToView = 1; }

private:
	IParticleVertexCreator* m_pVertexCreator;   //!< Particle object which computes vertices.
	SRenderVertices         m_RenderVerts;
	uint32                  m_nFirstVertex;
	uint32                  m_nFirstIndex;
	uint32                  m_allocId;
	uint16                  m_nThreadId;
	uint8                   m_addedToView;
};

#endif  // __CREPARTICLE_H__
