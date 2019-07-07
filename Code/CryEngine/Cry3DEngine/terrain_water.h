// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <CryRenderer/RenderElements/CREWaterVolume.h>

#define CYCLE_BUFFERS_NUM 4

class COcean final : public IRenderNode, public Cry3DEngineBase
{
public:
	COcean(IMaterial* pMat);
	~COcean();

	void          Create(const SRenderingPassInfo& passInfo);
	void          Update(const SRenderingPassInfo& passInfo);
	void          Render(const SRenderingPassInfo& passInfo);

	virtual bool  IsVisible(const AABB& nodeBox, const float nodeDistance, const SRenderingPassInfo& passInfo) const override;

	void          SetLastFov(float fLastFov) { m_fLastFov = fLastFov; }
	static void   SetTimer(ITimer* pTimer);

	static float  GetWave(const Vec3& pPosition, int32 nFrameID);
	static uint32 GetVisiblePixelsCount();
	int32         GetMemoryUsage();

	// fake IRenderNode implementation
	virtual EERType          GetRenderNodeType() const                                      override { return eERType_WaterVolume; }
	virtual const char*      GetEntityClassName(void) const                                 override { return "Ocean"; }
	virtual const char*      GetName(void) const                                            override { return "Ocean"; }
	virtual Vec3             GetPos(bool) const override;
	virtual void             Render(const SRendParams&, const SRenderingPassInfo& passInfo) override {}
	virtual IPhysicalEntity* GetPhysics(void) const                                         override { return 0; }
	virtual void             SetPhysics(IPhysicalEntity*)                                   override {}
	virtual void             SetMaterial(IMaterial* pMat)                                   override { m_pMaterial = pMat; }
	virtual IMaterial*       GetMaterial(Vec3* pHitPos = NULL) const override;
	virtual IMaterial*       GetMaterialOverride() const                                    override { return m_pMaterial; }
	virtual float            GetMaxViewDist() const                                         override { return 1000000.f; }
	virtual void             GetMemoryUsage(ICrySizer* pSizer) const                        override {}
	virtual const AABB       GetBBox() const                                                override { return AABB(Vec3(-1000000.f, -1000000.f, -1000000.f), Vec3(1000000.f, 1000000.f, 1000000.f)); }
	virtual void             SetBBox(const AABB& WSBBox)                                    override {}
	virtual void             FillBBox(AABB& aabb) const                                     override { aabb = GetBBox(); }
	virtual void             OffsetPosition(const Vec3& delta) override;

private:

	void RenderFog(const SRenderingPassInfo& passInfo);
	void RenderBottomCap(const SRenderingPassInfo& passInfo);

private:

	// Ocean data
	IMaterial*                m_pMaterial;

	int32                     m_nPrevGridDim;
	uint32                    m_nVertsCount;
	uint32                    m_nIndicesCount;

	int32                     m_nTessellationType;
	int32                     m_nTessellationTiles;

	// Ocean bottom cap data
	_smart_ptr<IMaterial>     m_pBottomCapMaterial;
	_smart_ptr<IRenderMesh>   m_pBottomCapRenderMesh;

	PodArray<SVF_P3F_C4B_T2F> m_pBottomCapVerts;
	PodArray<vtx_idx>         m_pBottomCapIndices;

	// Visibility data
	CCamera                  m_Camera;
	class CREOcclusionQuery* m_pREOcclusionQueries[CYCLE_BUFFERS_NUM];
	IShader*                 m_pShaderOcclusionQuery;
	float                    m_fLastFov;
	mutable float            m_fLastVisibleFrameTime;
	int32                    m_nLastVisibleFrameId;
	static uint32            m_nVisiblePixelsCount;

	float                    m_fRECustomData[12];           // used for passing data to renderer
	float                    m_fREOceanBottomCustomData[8]; // used for passing data to renderer
	bool                     m_bOceanFFT;

	// Ocean fog related members
	CREWaterVolume::SParams      m_wvParams[RT_COMMAND_BUF_COUNT];
	CREWaterVolume::SOceanParams m_wvoParams[RT_COMMAND_BUF_COUNT];

	_smart_ptr<IMaterial>        m_pFogIntoMat;
	_smart_ptr<IMaterial>        m_pFogOutofMat;
	_smart_ptr<IMaterial>        m_pFogIntoMatLowSpec;
	_smart_ptr<IMaterial>        m_pFogOutofMatLowSpec;

	CREWaterVolume*              m_pWVRE[RT_COMMAND_BUF_COUNT];
	std::vector<SVF_P3F_C4B_T2F> m_wvVertices[RT_COMMAND_BUF_COUNT];
	std::vector<uint16>          m_wvIndices[RT_COMMAND_BUF_COUNT];

	static ITimer*               m_pOceanTimer;
	static CREWaterOcean*        m_pOceanRE;

};
