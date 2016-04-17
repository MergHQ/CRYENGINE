// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __CRECLIENTPOLY_H__
#define __CRECLIENTPOLY_H__

//=============================================================

struct SClientPolyStat
{
	int NumOccPolys;
	int NumRendPolys;
	int NumVerts;
	int NumIndices;
};

class CREClientPoly : public CRendElementBase
{
public:
	enum eFlags
	{
		efAfterWater = 1 << 0,
		efShadowGen  = 1 << 1,
	};
	SShaderItem            m_Shader;
	CRenderObject*         m_pObject;
	short                  m_sNumVerts;
	short                  m_sNumIndices;
	byte                   m_nCPFlags;
	int                    m_nOffsVert;
	int                    m_nOffsTang;
	int                    m_nOffsInd;

	SRendItemSorter        rendItemSorter;
	static SClientPolyStat mRS;
	static void mfPrintStat();

public:
	CREClientPoly()
		: m_Shader()
		, m_pObject(nullptr)
		, m_sNumVerts(0)
		, m_sNumIndices(0)
		, m_nCPFlags(0)
		, m_nOffsVert(0)
		, m_nOffsTang(0)
		, m_nOffsInd(0)
	{
		mfSetType(eDATA_ClientPoly);
		mfUpdateFlags(FCEF_TRANSFORM);
	}

	virtual ~CREClientPoly() {};

	virtual void              mfPrepare(bool bCheckOverflow);
	virtual CRendElementBase* mfCopyConstruct(void);

	virtual void              GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};

#endif  // __CRECLIENTPOLY2D_H__
