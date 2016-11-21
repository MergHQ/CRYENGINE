// Copyright 2001-2015 Crytek GmbH. All rights reserved.

#pragma once
#include <array>

class CComputeRenderPass
{
public:
	enum EPassFlags
	{
		eFlags_None = 0,
		eFlags_ReflectConstantBuffersFromShader = BIT(0)
	};

	CComputeRenderPass(EPassFlags flags = eFlags_None);

#define ASSIGN_VALUE(dst, src, dirtyFlag) \
  if ((dst) != (src))                     \
    m_dirtyMask |= (dirtyFlag);           \
  (dst) = (src);

	bool InputChanged(int var0 = 0, int var1 = 0, int var2 = 0, int var3 = 0)
	{
		bool bChanged = m_dirtyMask > 0 || m_pResources->IsDirty() ||
		                !m_pPipelineState || !m_pPipelineState->IsValid() ||
		                m_currentPsoUpdateCount != m_pPipelineState->GetUpdateCount() ||
		                var0 != m_inputVars[0] || var1 != m_inputVars[1] ||
		                var2 != m_inputVars[2] || var3 != m_inputVars[3];

		if (bChanged)
		{
			m_inputVars[0] = var0;
			m_inputVars[1] = var1;
			m_inputVars[2] = var2;
			m_inputVars[3] = var3;
		}

		return bChanged;
	}

	void SetOutputUAV(uint32 slot, CTexture* pTexture)
	{
		m_pResources->SetTexture(slot, pTexture, SResourceView::DefaultUnordererdAccessView, EShaderStage_Compute);
		m_dirtyMask |= m_pResources->IsDirty() ? eDirty_Resources : eDirty_None;
	}

	void SetOutputUAV(uint32 slot, CGpuBuffer* pBuffer)
	{
		m_pResources->SetBuffer(slot, pBuffer ? *pBuffer : CGpuBuffer(), true, EShaderStage_Compute);
		m_dirtyMask |= m_pResources->IsDirty() ? eDirty_Resources : eDirty_None;
	}

	void SetDispatchSize(uint32 sizeX, uint32 sizeY, uint32 sizeZ)
	{
		m_dispatchSizeX = sizeX;
		m_dispatchSizeY = sizeY;
		m_dispatchSizeZ = sizeZ;
	}

	void SetTechnique(CShader* pShader, const CCryNameTSCRC& techName, uint64 rtMask)
	{
		ASSIGN_VALUE(m_pShader, pShader, eDirty_Technique);
		ASSIGN_VALUE(m_techniqueName, techName, eDirty_Technique);
		ASSIGN_VALUE(m_rtMask, rtMask, eDirty_Technique);

		if (m_pPipelineState && (!m_pPipelineState->IsValid() || m_currentPsoUpdateCount != m_pPipelineState->GetUpdateCount()))
		{
			m_dirtyMask |= eDirty_Technique;
		}

		if (m_flags & eFlags_ReflectConstantBuffersFromShader)
		{
			if (m_dirtyMask & eDirty_Technique)
			{
				m_constantManager.InitShaderReflection(pShader, techName, rtMask);
			}
		}
	}

	void SetFlags(EPassFlags flags)
	{
		if (m_flags != flags)
		{
			const bool bHaveShaderReflection = m_flags & eFlags_ReflectConstantBuffersFromShader;
			const bool bWantShaderReflection =   flags & eFlags_ReflectConstantBuffersFromShader;

			if (!bHaveShaderReflection && bWantShaderReflection && m_pShader)
			{
				m_constantManager.InitShaderReflection(m_pShader, m_techniqueName, m_rtMask);
				m_rtMask |= eDirty_Resources;
			}
			else if (bHaveShaderReflection && !bWantShaderReflection)
			{
				m_constantManager.ReleaseShaderReflection();
				m_rtMask |= eDirty_Resources;
			}

			m_flags = flags;
		}
	}

	void SetTexture(uint32 slot, CTexture* pTexture, SResourceView::KeyType resourceViewID = SResourceView::DefaultView)
	{
		m_pResources->SetTexture(slot, pTexture, resourceViewID, EShaderStage_Compute);
		m_dirtyMask |= m_pResources->IsDirty() ? eDirty_Resources : eDirty_None;
	}

	void SetTextureSamplerPair(uint32 slot, CTexture* pTex, int32 sampler, SResourceView::KeyType resourceViewID = SResourceView::DefaultView)
	{
		m_pResources->SetTexture(slot, pTex, resourceViewID, EShaderStage_Compute);
		m_pResources->SetSampler(slot, sampler, EShaderStage_Compute);
	}

	void SetBuffer(uint32 slot, CGpuBuffer* pBuffer)
	{
		m_pResources->SetBuffer(slot, *pBuffer, false, EShaderStage_Compute);
		m_dirtyMask |= m_pResources->IsDirty() ? eDirty_Resources : eDirty_None;
	}

	void SetSampler(uint32 slot, int32 sampler)
	{
		m_pResources->SetSampler(slot, sampler, EShaderStage_Compute);
		m_dirtyMask |= m_pResources->IsDirty() ? eDirty_Resources : eDirty_None;
	}

	void SetInlineConstantBuffer(uint32 slot, _smart_ptr<CConstantBuffer> pConstantBuffer)
	{
		if (m_constantManager.SetTypedConstantBuffer(EConstantBufferShaderSlot(slot), pConstantBuffer, EShaderStage_Compute))
			m_dirtyMask |= eDirty_Resources;
	}

	void SetConstant(const CCryNameR& paramName, const Vec4 param)
	{
		m_constantManager.SetNamedConstant(paramName, param, eHWSC_Compute);
	}

	void SetConstantArray(const CCryNameR& paramName, const Vec4 params[], uint32 numParams)
	{
		m_constantManager.SetNamedConstantArray(paramName, params, numParams, eHWSC_Compute);
	}

	void PrepareResourcesForUse(CDeviceCommandListRef RESTRICT_REFERENCE commandList, ::EShaderStage srvUsage = EShaderStage_Compute);

	void BeginConstantUpdate();

	void BeginRenderPass(CDeviceCommandListRef RESTRICT_REFERENCE commandList);
	void Dispatch(CDeviceCommandListRef RESTRICT_REFERENCE commandList, ::EShaderStage srvUsage);
	void EndRenderPass(CDeviceCommandListRef RESTRICT_REFERENCE commandList);

	// Executes this pass as if it's the only participant of a compute stage (see CSceneGBufferStage + CSceneRenderPass)
	void Execute(CDeviceCommandListRef RESTRICT_REFERENCE commandList, ::EShaderStage srvUsage = EShaderStage_Compute);

protected:

#undef ASSIGN_VALUE

private:
	enum EDirtyFlags
	{
		eDirty_Technique = BIT(0),
		eDirty_Resources = BIT(1),

		eDirty_None      = 0,
		eDirty_All       = eDirty_Technique | eDirty_Resources
	};

	uint Compile();

	typedef SDeviceObjectHelpers::CShaderConstantManager ConstantManager;
private:

	int                      m_inputVars[4];

	EPassFlags               m_flags;
	uint32                   m_dirtyMask;
	CShader*                 m_pShader;
	CCryNameTSCRC            m_techniqueName;
	uint64                   m_rtMask;
	uint32                   m_dispatchSizeX;
	uint32                   m_dispatchSizeY;
	uint32                   m_dispatchSizeZ;
	CDeviceResourceSetPtr    m_pResources;
	CDeviceResourceLayoutPtr m_pResourceLayout;
	CDeviceComputePSOPtr     m_pPipelineState;
	int                      m_currentPsoUpdateCount;

	ConstantManager          m_constantManager;

	bool                     m_bPendingConstantUpdate;

	uint64                   m_prevRTMask;

	SProfilingStats          m_profilingStats;
};
