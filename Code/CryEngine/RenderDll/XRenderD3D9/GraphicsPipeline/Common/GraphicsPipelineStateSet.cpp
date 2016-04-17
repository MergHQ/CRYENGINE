// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GraphicsPipelineStateSet.h"
#include "GraphicsPipelineStage.h"
#include "DriverD3D.h"
#include "xxhash.h"

SGraphicsPipelineStateDescription::SGraphicsPipelineStateDescription(
  CRenderObject* pObj,
  const SShaderItem& _shaderItem,
  EShaderTechniqueID _technique,
  EVertexFormat _vertexFormat,
  uint32 _streamMask,
  int _primitiveType)
	: _dummy(0)
{
	shaderItem = _shaderItem;
	technique = _technique;
	objectFlags = pObj->m_ObjFlags;
	objectFlags_MDV = pObj->m_nMDV;
	objectRuntimeMask = pObj->m_nRTMask;
	vertexFormat = _vertexFormat;
	streamMask = _streamMask;
	primitiveType = _primitiveType;

	if ((pObj->m_ObjFlags & FOB_SKINNED) && CRenderer::CV_r_usehwskinning && !CRenderer::CV_r_character_nodeform)
	{
		SSkinningData* pSkinningData = NULL;
		SRenderObjData* pOD = pObj->GetObjData();
		if (pOD && (pSkinningData = pOD->m_pSkinningData))
		{
			if (pSkinningData->nHWSkinningFlags & eHWS_SkinnedLinear)
				objectRuntimeMask |= (g_HWSR_MaskBit[HWSR_SKELETON_SSD_LINEAR]);
			else
				objectRuntimeMask |= (g_HWSR_MaskBit[HWSR_SKELETON_SSD]);
		}
	}
}

uint64 CGraphicsPipelineStateLocalCache::GetHashKey(const SGraphicsPipelineStateDescription& desc) const
{
	// Simple sum of all elements
	uint64 key = XXH64(&desc, sizeof(desc), 0);
	return key;
}

bool CGraphicsPipelineStateLocalCache::Find(const SGraphicsPipelineStateDescription& desc, DevicePipelineStatesArray& out)
{
	uint64 key = GetHashKey(desc);
	for (auto it = m_states.begin(); it != m_states.end(); ++it)
	{
		if (it->stateHashKey == key && it->description == desc)
		{
			out = it->m_pipelineStates;
			return true;
		}
	}
	return false;
}

void CGraphicsPipelineStateLocalCache::Put(const SGraphicsPipelineStateDescription& desc, const DevicePipelineStatesArray& states)
{
	// Cache this state locally.
	CachedState cache;
	cache.stateHashKey = GetHashKey(desc);
	cache.description = desc;
	cache.m_pipelineStates = states;
	m_states.push_back(cache);
}
