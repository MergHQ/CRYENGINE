// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "PrimitiveRenderPass.h"

class CFullscreenPass : public CPrimitiveRenderPass
{
public:
	CFullscreenPass();
	~CFullscreenPass();

	bool InputChanged(int var0 = 0, int var1 = 0, int var2 = 0, int var3 = 0)
	{
		bool bChanged = m_primitive.IsDirty() ||
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

	ILINE void SetTechnique(CShader* pShader, const CCryNameTSCRC& techName, uint64 rtMask)
	{
		m_primitive.SetTechnique(pShader, techName, rtMask);
	}

	ILINE void SetTexture(uint32 slot, CTexture* pTexture, SResourceView::KeyType resourceViewID = SResourceView::DefaultView)
	{
		m_primitive.SetTexture(slot, pTexture, resourceViewID);
	}

	ILINE void SetSampler(uint32 slot, int32 sampler)
	{
		m_primitive.SetSampler(slot, sampler);
	}

	ILINE void SetTextureSamplerPair(uint32 slot, CTexture* pTex, int32 sampler, SResourceView::KeyType resourceViewID = SResourceView::DefaultView)
	{
		m_primitive.SetTexture(slot, pTex, resourceViewID);
		m_primitive.SetSampler(slot, sampler);
	}

	ILINE void SetState(int state)
	{
		m_primitive.SetRenderState(state);
	}

	ILINE void SetStencilState(int state, uint8 stencilRef, uint8 stencilReadMask = 0xFF, uint8 stencilWriteMask = 0xFF)
	{
		m_primitive.SetStencilState(state, stencilRef, stencilReadMask, stencilWriteMask);
	}

	void SetRequirePerViewConstantBuffer(bool bRequirePerViewCB)
	{
		m_bRequirePerViewCB = bRequirePerViewCB;
	}

	void SetRequireWorldPos(bool bRequireWPos, f32 clipSpaceZ = 0.0f)
	{
		m_bRequireWorldPos = bRequireWPos;
		m_clipZ = clipSpaceZ;
	}

	void SetConstant(EHWShaderClass shaderClass, const CCryNameR& paramName, const Vec4 param)
	{
		m_primitive.GetConstantManager().SetNamedConstant(shaderClass, paramName, param);
	}

	void SetConstantArray(EHWShaderClass shaderClass, const CCryNameR& paramName, const Vec4 params[], uint32 numParams)
	{
		m_primitive.GetConstantManager().SetNamedConstantArray(shaderClass, paramName, params, numParams);
	}

	void SetInlineConstantBuffer(EConstantBufferShaderSlot shaderSlot, CConstantBuffer* pBuffer, EShaderStage shaderStages = EShaderStage_Pixel)
	{
		m_primitive.SetInlineConstantBuffer(shaderSlot, pBuffer, shaderStages);
	}

	void BeginConstantUpdate();
	void Execute();

private:
	int                      m_inputVars[4];

	bool                     m_bRequirePerViewCB;
	bool                     m_bRequireWorldPos;
	bool                     m_bPendingConstantUpdate;

	f32                      m_clipZ;        // only work for WPos
	buffer_handle_t          m_vertexBuffer; // only required for WPos
	uint64                   m_prevRTMask;

	CRenderPrimitive m_primitive;
};
