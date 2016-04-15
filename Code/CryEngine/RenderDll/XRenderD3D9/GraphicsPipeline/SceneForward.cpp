// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SceneForward.h"
#include "DriverD3D.h"

void CSceneForwardStage::Init()
{
}

void CSceneForwardStage::Prepare(CRenderView* pRenderView)
{
}

void CSceneForwardStage::Execute_Opaque()
{
	PROFILE_LABEL_SCOPE("OPAQUE_PASSES");

	CD3D9Renderer* pRenderer = gcpRendD3D;

	void (* pRenderFunc)() = &pRenderer->FX_FlushShader_General;
	pRenderer->m_RP.m_PersFlags2 |= RBPF2_FORWARD_SHADING_PASS;

	// Note: Eye overlay writes to diffuse color buffer for eye shader reading
	pRenderer->FX_ProcessEyeOverlayRenderLists(EFSLIST_EYE_OVERLAY, pRenderFunc, true);

	{
		PROFILE_LABEL_SCOPE("FORWARD_OPAQUE");
		pRenderer->GetTiledShading().BindForwardShadingResources(NULL);
		pRenderer->FX_ProcessRenderList(EFSLIST_FORWARD_OPAQUE, pRenderFunc, true);
		pRenderer->GetTiledShading().UnbindForwardShadingResources();
	}

	{
		PROFILE_LABEL_SCOPE("TERRAINLAYERS");
		pRenderer->FX_ProcessRenderList(EFSLIST_TERRAINLAYER, pRenderFunc, true);
	}

	{
		PROFILE_LABEL_SCOPE("FORWARD_DECALS");
		pRenderer->FX_ProcessRenderList(EFSLIST_DECAL, pRenderFunc, true);
	}

	pRenderer->FX_ProcessSkinRenderLists(EFSLIST_SKIN, pRenderFunc, true);

	pRenderer->m_RP.m_PersFlags2 &= ~RBPF2_FORWARD_SHADING_PASS;
}

void CSceneForwardStage::Execute_TransparentBelowWater()
{
	PROFILE_LABEL_SCOPE("TRANSPARENT_BW");

	CD3D9Renderer* pRenderer = gcpRendD3D;

	void (* pRenderFunc)() = &pRenderer->FX_FlushShader_General;

	if (!SRendItem::IsListEmpty(EFSLIST_TRANSP) && (SRendItem::BatchFlags(EFSLIST_TRANSP) & FB_BELOW_WATER))
	{
		pRenderer->FX_ScreenStretchRect(CTexture::s_ptexCurrSceneTarget);
	}

	pRenderer->GetTiledShading().BindForwardShadingResources(NULL);
	pRenderer->FX_ProcessRenderList(EFSLIST_TRANSP, pRenderFunc, true, FB_BELOW_WATER, 0);
	pRenderer->GetTiledShading().UnbindForwardShadingResources();
}

void CSceneForwardStage::Execute_TransparentAboveWater()
{
	PROFILE_LABEL_SCOPE("TRANSPARENT_AW");

	CD3D9Renderer* pRenderer = gcpRendD3D;

	void (* pRenderFunc)() = &pRenderer->FX_FlushShader_General;

	if (!SRendItem::IsListEmpty(EFSLIST_TRANSP))
	{
		pRenderer->FX_ScreenStretchRect(CTexture::s_ptexCurrSceneTarget);
	}

	pRenderer->GetTiledShading().BindForwardShadingResources(NULL);
	pRenderer->FX_ProcessRenderList(EFSLIST_TRANSP, pRenderFunc, true, FB_GENERAL, FB_BELOW_WATER);
	pRenderer->GetTiledShading().UnbindForwardShadingResources();
}
