// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CDeviceCommandList;
class CWaterStage;
namespace water
{
struct SCompiledWaterOcean;
}

class CREWaterOcean : public CRenderElement
{
public:
	struct SWaterOceanParam
	{
		bool bWaterOceanFFT;

		SWaterOceanParam() : bWaterOceanFFT(false) {}
	};

public:
	CREWaterOcean();
	virtual ~CREWaterOcean();

	virtual void Release(bool bForce = false) override;

	virtual void mfGetPlane(Plane& pl) override;

	virtual void GetMemoryUsage(ICrySizer* pSizer) const override
	{
		pSizer->AddObject(this, sizeof(*this));
	}

	virtual bool            Compile(CRenderObject* pObj) override;
	virtual void            DrawToCommandList(CRenderObject* pObj, const struct SGraphicsPipelinePassContext& ctx) override;

	virtual void            Create(uint32 nVerticesCount, SVF_P3F_C4B_T2F* pVertices, uint32 nIndicesCount, const void* pIndices, uint32 nIndexSizeof);
	virtual Vec4*           GetDisplaceGrid() const;

	virtual SHRenderTarget* GetReflectionRenderTarget();

public:
	SWaterOceanParam m_oceanParam[RT_COMMAND_BUF_COUNT];

private:
	void FrameUpdate();
	void ReleaseOcean();

	void PrepareForUse(water::SCompiledWaterOcean& compiledObj, bool bInstanceOnly, CDeviceCommandList& RESTRICT_REFERENCE commandList) const;

	void UpdatePerInstanceResourceSet(water::SCompiledWaterOcean& RESTRICT_REFERENCE compiledObj, const SWaterOceanParam& oceanParam, const CWaterStage& waterStage);
	void UpdatePerInstanceCB(water::SCompiledWaterOcean& RESTRICT_REFERENCE compiledObj, const CRenderObject& renderObj) const;
	void UpdateVertex(water::SCompiledWaterOcean& compiledObj, int32 primType);

private:
	std::unique_ptr<water::SCompiledWaterOcean> m_pCompiledObject;

	std::unique_ptr<SHRenderTarget>             m_pRenderTarget;

	stream_handle_t                             m_vertexBufferHandle;
	stream_handle_t                             m_indexBufferHandle;

	uint32 m_nVerticesCount;
	uint32 m_nIndicesCount;
	uint32 m_nIndexSizeof;

};
