// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CREMeshImpl final : public CREMesh
{
public:

	uint       m_nPatchIDOffset;

	CREMeshImpl()
		: m_nPatchIDOffset(0)
	{}

	virtual ~CREMeshImpl()
	{}

public:
	struct CRenderChunk* mfGetMatInfo();
	TRenderChunkArray*   mfGetMatInfoList();
	int                  mfGetMatId();

	bool                 mfIsHWSkinned()
	{
		return (m_Flags & FCEF_SKINNED) != 0;
	}

	void* mfGetPointer(ESrcPointer ePT, int* Stride, EParamType Type, ESrcPointer Dst, EStreamMasks StreamMask);
	bool  mfUpdate(InputLayoutHandle eVertFormat, EStreamMasks StreamMask, bool bTessellation = false);
	void  mfGetBBox(AABB& bb) const;

	int   Size()
	{
		int nSize = sizeof(*this);
		return nSize;
	}
	void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

	bool          BindRemappedSkinningData(uint32 guid);

	bool          GetGeometryInfo(SGeometryInfo& geomInfo, bool bSupportTessellation = false);
	InputLayoutHandle GetVertexFormat() const;
	bool          Compile(CRenderObject* pObj, uint64 objFlags, ERenderElementFlags elmFlags, const AABB &localAABB, CRenderView *pRenderView, bool updateInstanceDataOnly);
	void          DrawToCommandList(CRenderObject* pObj, const SGraphicsPipelinePassContext& ctx, CDeviceCommandList* commandList);

	//protected:
	//	CREMeshImpl(CREMeshImpl&);
	//	CREMeshImpl& operator=(CREMeshImpl&);
};