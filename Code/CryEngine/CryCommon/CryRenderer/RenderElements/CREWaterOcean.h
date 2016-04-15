// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef _CREWATEROCEAN_
#define _CREWATEROCEAN_

class CWater;

class CREWaterOcean : public CRendElementBase
{
public:
	CREWaterOcean();
	virtual ~CREWaterOcean();

	virtual void  mfPrepare(bool bCheckOverflow);
	virtual bool  mfDraw(CShader* ef, SShaderPass* sfm);
	virtual void  mfGetPlane(Plane& pl);

	virtual void  Create(uint32 nVerticesCount, SVF_P3F_C4B_T2F* pVertices, uint32 nIndicesCount, const void* pIndices, uint32 nIndexSizeof);
	void          ReleaseOcean();

	virtual Vec3  GetPositionAt(float x, float y) const;
	virtual Vec4* GetDisplaceGrid() const;

	virtual void  GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
private:

	uint32 m_nVerticesCount;
	uint32 m_nIndicesCount;
	uint32 m_nIndexSizeof;

	void*  m_pVertDecl;
	void*  m_pVertices;
	void*  m_pIndices;

private:

	void UpdateFFT();
	void FrameUpdate();

};

#endif
