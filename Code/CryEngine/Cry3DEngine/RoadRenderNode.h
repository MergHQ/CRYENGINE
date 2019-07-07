// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CRoadRenderNode final : public IRoadRenderNode, public Cry3DEngineBase
{
public:
	// SDynamicData and SData are helpers for serialization
	struct SData
	{
		float arrTexCoors[2];
		float arrTexCoorsGlobal[2];

		AABB  worldSpaceBBox;

		// Don't use vtx_idx, we need to keep levels exported on PC loadable on lesser platforms
		uint32 numVertices;
		uint32 numIndices;
		uint32 numTangents;

		uint32 physicsGeometryCount;

		// Number of vertices that can be used to fully rebuild the road data
		uint32 sourceVertexCount;

		AUTO_STRUCT_INFO;
	};

	struct SPhysicsGeometryParams
	{
		Vec3 size;
		Vec3 pos;
		Quat q;

		AUTO_STRUCT_INFO;
	};

	struct SDynamicData
	{
		PodArray<SVF_P3F_C4B_T2S>        vertices;
		PodArray<vtx_idx>                indices;
		PodArray<SPipTangents>           tangents;

		PodArray<SPhysicsGeometryParams> physicsGeometry;
	};

public:
	CRoadRenderNode();
	virtual ~CRoadRenderNode();

	// IRoadRenderNode implementation
	virtual void                SetVertices(const Vec3* pVerts, int nVertsNum, float fTexCoordBegin, float fTexCoordEnd, float fTexCoordBeginGlobal, float fTexCoordEndGlobal) override;
	virtual void                SetSortPriority(uint8 sortPrio) override;
	virtual void                SetIgnoreTerrainHoles(bool bVal) override;
	virtual void                SetPhysicalize(bool bVal) override { if (m_bPhysicalize != bVal) { m_bPhysicalize = bVal; ScheduleRebuild(true); } }

	// IRenderNode implementation
	virtual const char*         GetEntityClassName(void) const override { return "RoadObjectClass"; }
	virtual const char*         GetName(void) const            override { return "RoadObjectName"; }
	virtual Vec3                GetPos(bool) const override;
	virtual void                Render(const SRendParams& RendParams, const SRenderingPassInfo& passInfo) override;

	virtual IPhysicalEntity*    GetPhysics(void) const             override { return m_pPhysEnt; }
	virtual void                SetPhysics(IPhysicalEntity* pPhys) override { m_pPhysEnt = pPhys; }

	virtual void                SetMaterial(IMaterial* pMat) override;
	virtual IMaterial*          GetMaterial(Vec3* pHitPos = NULL) const override;
	virtual IMaterial*          GetMaterialOverride() const override { return m_pMaterial; }
	virtual float               GetMaxViewDist() const override;
	virtual EERType             GetRenderNodeType() const override { return eERType_Road; }
	virtual struct IRenderMesh* GetRenderMesh(int nLod) const override { return nLod == 0 ? m_pRenderMesh.get() : NULL; }
	virtual void                GetMemoryUsage(ICrySizer* pSizer) const override;
	virtual const AABB          GetBBox() const             override { return m_serializedData.worldSpaceBBox; }
	virtual void                SetBBox(const AABB& WSBBox) override { m_serializedData.worldSpaceBBox = WSBBox; }
	virtual void                FillBBox(AABB& aabb) const override { aabb = GetBBox(); }
	virtual void                OffsetPosition(const Vec3& delta) override;
	virtual void                GetClipPlanes(Plane* pPlanes, int nPlanesNum, int nVertId) override;
	virtual void                GetTexCoordInfo(float* pTexCoordInfo) override;
	virtual uint8               GetSortPriority() const override { return m_sortPrio; }
	virtual bool                CanExecuteRenderAsJob() const override;

	virtual void                SetLayerId(uint16 nLayerId) override { m_nLayerId = nLayerId; Get3DEngine()->C3DEngine::UpdateObjectsLayerAABB(this); }
	virtual uint16              GetLayerId() const          override { return m_nLayerId; }

	static bool                 ClipTriangle(PodArray<Vec3>& lstVerts, PodArray<vtx_idx>& lstInds, int nStartIdxId, Plane* pPlanes);
	using IRenderNode::Physicalize;
	virtual void                Dephysicalize(bool bKeepIfReferenced = false) override;
	void                        Compile();
	void                        ScheduleRebuild(bool bFullRebuild);
	void                        OnTerrainChanged();

	_smart_ptr<IRenderMesh> m_pRenderMesh;
	_smart_ptr<IMaterial>   m_pMaterial;
	PodArray<Vec3>          m_arrVerts;

	SData                   m_serializedData;
	SDynamicData            m_dynamicData;

	// Set when scheduling rebuild, true if the road was changed and we need to recompile it
	bool             m_bRebuildFull;

	uint8            m_sortPrio;
	bool             m_bIgnoreTerrainHoles;
	bool             m_bPhysicalize;

	IPhysicalEntity* m_pPhysEnt;

	uint16           m_nLayerId;

	// Temp buffers used during road mesh creation
	static PodArray<Vec3>            s_tempVertexPositions;
	static PodArray<vtx_idx>         s_tempIndices;

	static PodArray<SPipTangents>    s_tempTangents;
	static PodArray<SVF_P3F_C4B_T2S> s_tempVertices;

	static CPolygonClipContext       s_tmpClipContext;

	static void GetMemoryUsageStatic(ICrySizer* pSizer)
	{
		SIZER_COMPONENT_NAME(pSizer, "RoadRenderNodeStaticData");

		pSizer->AddObject(s_tempVertexPositions);
		pSizer->AddObject(s_tempIndices);

		pSizer->AddObject(s_tempTangents);
		pSizer->AddObject(s_tempVertices);

		pSizer->AddObject(s_tmpClipContext);
	}

	static void FreeStaticMemoryUsage()
	{
		s_tempVertexPositions.Reset();
		s_tempIndices.Reset();

		s_tempTangents.Reset();
		s_tempVertices.Reset();

		s_tmpClipContext.Reset();
	}
};
