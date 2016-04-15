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
		bool bChanged = GetPrimitive().IsDirty() ||
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

	ILINE void SetTechnique(CShader* pShader, CCryNameTSCRC& techName, uint64 rtMask)
	{
		GetPrimitive().SetTechnique(pShader, techName, rtMask);
	}

	ILINE void SetTexture(uint32 slot, CTexture* pTexture, SResourceView::KeyType resourceViewID = SResourceView::DefaultView)
	{
		GetPrimitive().SetTexture(slot, pTexture, resourceViewID);
	}

	ILINE void SetSampler(uint32 slot, int32 sampler)
	{
		GetPrimitive().SetSampler(slot, sampler);
	}

	ILINE void SetTextureSamplerPair(uint32 slot, CTexture* pTex, int32 sampler, SResourceView::KeyType resourceViewID = SResourceView::DefaultView)
	{
		GetPrimitive().SetTexture(slot, pTex, resourceViewID);
		GetPrimitive().SetSampler(slot, sampler);
	}

	ILINE void SetState(int state)
	{
		GetPrimitive().SetRenderState(state);
	}

	void SetRequirePerViewConstantBuffer(bool bRequirePerViewCB)
	{
		m_bRequirePerViewCB = bRequirePerViewCB;
	}

	void SetRequireWorldPos(bool bRequireWPos)
	{
		m_bRequireWorldPos = bRequireWPos;
	}

	void BeginConstantUpdate();
	void Execute();

private:
	ILINE CCompiledRenderPrimitive& GetPrimitive() { return m_primitives[0]; }

	int             m_inputVars[4];

	bool            m_bRequirePerViewCB;
	bool            m_bRequireWorldPos;
	bool            m_bValidConstantBuffers;
	bool            m_bPendingConstantUpdate;

	buffer_handle_t m_vertexBuffer; // only required for WPos
	uint64          m_prevRTMask;
};
