// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#ifdef INCLUDE_SCALEFORM_SDK

	#include "GRendererXRender.h"
	#include "GTextureXRender.h"
	#include "SharedStates.h"

	#include <CrySystem/ISystem.h>
	#include <CryRenderer/IRenderer.h>
	#include <CryRenderer/IStereoRenderer.h>
	#include <CrySystem/IConsole.h>

	#if defined(_DEBUG)

		#define _RECORD_CMD_PREFIX            \
		  IF (m_pCmdBuf && IsMainThread(), 0) \
		  {                                   \
		    size_t startPos = (size_t) -1;

		#define _RECORD_CMD(cmd)        \
		  assert(cmd != GRCBA_Nop);     \
		  m_pCmdBuf->Write(cmd);        \
		  startPos = m_pCmdBuf->Size(); \
		  m_pCmdBuf->Write((size_t) -1);

		#define _RECORD_CMD_ARG(arg) \
		  m_pCmdBuf->Write(arg);

		#define _RECORD_CMD_DATA(data, size) \
		  m_pCmdBuf->Write(data, size);

		#define _RECORD_CMD_POSTFIX(ret)                            \
		  assert(startPos != -1);                                   \
		  m_pCmdBuf->Patch(startPos, m_pCmdBuf->Size() - startPos); \
		  return ret;                                               \
		  }

		#define _PLAYBACK_CMD_SUFFIX              \
		  {                                       \
		    const size_t startPos = GetReadPos(); \
		    const size_t argDataSize = Read<size_t>();

		#define _PLAYBACK_CMD_POSTFIX                             \
		  const size_t argDataSizeRead = GetReadPos() - startPos; \
		  assert(argDataSizeRead == argDataSize);                 \
		  }

	#else // #if defined(_DEBUG)

		#define _RECORD_CMD_PREFIX            \
		  IF (m_pCmdBuf && IsMainThread(), 0) \
		  {

		#define _RECORD_CMD(cmd) \
		  m_pCmdBuf->Write(cmd); \

		#define _RECORD_CMD_ARG(arg) \
		  m_pCmdBuf->Write(arg);

		#define _RECORD_CMD_DATA(data, size) \
		  m_pCmdBuf->Write(data, size);

		#define _RECORD_CMD_POSTFIX(ret) \
		  return ret;                    \
		  }

		#define _PLAYBACK_CMD_SUFFIX \
		  {

		#define _PLAYBACK_CMD_POSTFIX \
		  }

	#endif // #if defined(_DEBUG)

	#define CMD_VOID_RETURN ((void) 0)

	#define RECORD_CMD(cmd, ret) \
	  _RECORD_CMD_PREFIX         \
	  _RECORD_CMD(cmd)           \
	  _RECORD_CMD_POSTFIX(ret)

	#define RECORD_CMD_1ARG(cmd, ret, arg0) \
	  _RECORD_CMD_PREFIX                    \
	  _RECORD_CMD(cmd)                      \
	  _RECORD_CMD_ARG(arg0)                 \
	  _RECORD_CMD_POSTFIX(ret)

	#define RECORD_CMD_2ARG(cmd, ret, arg0, arg1) \
	  _RECORD_CMD_PREFIX                          \
	  _RECORD_CMD(cmd)                            \
	  _RECORD_CMD_ARG(arg0)                       \
	  _RECORD_CMD_ARG(arg1)                       \
	  _RECORD_CMD_POSTFIX(ret)

	#define RECORD_CMD_3ARG(cmd, ret, arg0, arg1, arg2) \
	  _RECORD_CMD_PREFIX                                \
	  _RECORD_CMD(cmd)                                  \
	  _RECORD_CMD_ARG(arg0)                             \
	  _RECORD_CMD_ARG(arg1)                             \
	  _RECORD_CMD_ARG(arg2)                             \
	  _RECORD_CMD_POSTFIX(ret)

	#define RECORD_CMD_4ARG(cmd, ret, arg0, arg1, arg2, arg3) \
	  _RECORD_CMD_PREFIX                                      \
	  _RECORD_CMD(cmd)                                        \
	  _RECORD_CMD_ARG(arg0)                                   \
	  _RECORD_CMD_ARG(arg1)                                   \
	  _RECORD_CMD_ARG(arg2)                                   \
	  _RECORD_CMD_ARG(arg3)                                   \
	  _RECORD_CMD_POSTFIX(ret)

	#define RECORD_CMD_5ARG(cmd, ret, arg0, arg1, arg2, arg3, arg4) \
	  _RECORD_CMD_PREFIX                                            \
	  _RECORD_CMD(cmd)                                              \
	  _RECORD_CMD_ARG(arg0)                                         \
	  _RECORD_CMD_ARG(arg1)                                         \
	  _RECORD_CMD_ARG(arg2)                                         \
	  _RECORD_CMD_ARG(arg3)                                         \
	  _RECORD_CMD_ARG(arg4)                                         \
	  _RECORD_CMD_POSTFIX(ret)

	#define RECORD_CMD_6ARG(cmd, ret, arg0, arg1, arg2, arg3, arg4, arg5) \
	  _RECORD_CMD_PREFIX                                                  \
	  _RECORD_CMD(cmd)                                                    \
	  _RECORD_CMD_ARG(arg0)                                               \
	  _RECORD_CMD_ARG(arg1)                                               \
	  _RECORD_CMD_ARG(arg2)                                               \
	  _RECORD_CMD_ARG(arg3)                                               \
	  _RECORD_CMD_ARG(arg4)                                               \
	  _RECORD_CMD_ARG(arg5)                                               \
	  _RECORD_CMD_POSTFIX(ret)

struct FillTextureNULL
{
	static const GRenderer::FillTexture& Get() { return s_singleInst.m_ft; }

private:
	static const FillTextureNULL s_singleInst;

	GRenderer::FillTexture       m_ft;

	FillTextureNULL()
	{
		m_ft.pTexture = 0;
		m_ft.TextureMatrix.SetIdentity();
		m_ft.WrapMode = GRenderer::Wrap_Repeat;
		m_ft.SampleMode = GRenderer::Sample_Point;
	}
};

const FillTextureNULL FillTextureNULL::s_singleInst;

static inline void GTextureAddRef(GTexture* p)
{
	IF (p, 1)
		p->AddRef();
}

static inline void GTextureRelease(GTexture* p)
{
	IF (p, 1)
		p->Release();
}

	#define _RECORD_CMD_ARG_FILLTEX(pFill) \
	  IF (!pFill, 0)                       \
	    pFill = &FillTextureNULL::Get();   \
	  _RECORD_CMD_ARG(*pFill)              \
	  GTextureAddRef(pFill->pTexture);

//////////////////////////////////////////////////////////////////////////

class CCachedDataStore
{
public:
	static CCachedDataStore* Create(GRenderer* pRenderer, GRenderer::CacheProvider* pCache, GRenderer::CachedDataType type, const void* pData, size_t size)
	{
		CCachedDataStore* pStore = 0;
		if (pCache)
		{
			GRenderer::CachedData* pCD = pCache->GetCachedData(pRenderer);
			if (pCD)
			{
				pStore = (CCachedDataStore*) pCD->GetRendererData();
				assert(pStore);
				assert(pStore->m_type == type && pStore->m_pData == pData && pStore->m_size == size); // data was previously cached with different args and not properly released yet via ReleaseCachedData()
				pStore->AddRef();
			}
			else
			{
				pCD = pCache->CreateCachedData(type, pRenderer);
				assert(pCD);
				pStore = new CCachedDataStore(type, pData, size);
				assert(pStore);
				pStore->AddRef();
				pCD->SetRendererData(pStore);
			}
		}
		return pStore;
	}

public:
	GFC_MEMORY_REDEFINE_NEW(CCachedDataStore, GStat_Default_Mem)

public:
	void AddRef()
	{
		CryInterlockedIncrement(&m_refCnt);
	}

	void Release()
	{
		long refCnt = CryInterlockedDecrement(&m_refCnt);
		if (!refCnt)
			delete this;
	}

	void BackupAndRelease()
	{
		LockForBackup();

		if (m_refCnt > 1 && m_pData && m_size && !m_pDataCopy)
		{
			// Additional check for "!m_pDataCopy" added as ReleaseCachedData(), i.e. our only caller, can be called from
			// render thread due to deferred (final) Release(). Otherwise called from main only, i.e. Release() or Display()
			m_pDataCopy = GALLOC(m_size, GStat_Default_Mem);
			memcpy(m_pDataCopy, m_pData, m_size);
			m_pData = m_pDataCopy;
		}

		Unlock();

		Release();
	}

	const void* GetPtr()
	{
		assert(m_lock == 1);
		return m_pData;
	}

	void Lock()
	{
		LockInternal(1);
	}

	void Unlock()
	{
		m_lock = 0;
	}

private:
	void LockForBackup()
	{
		LockInternal(-1);
	}

	void LockInternal(const LONG lockVal)
	{
		while (true)
		{
			if (0 == CryInterlockedCompareExchange(&m_lock, lockVal, 0))
				break;
		}
	}

private:
	CCachedDataStore(GRenderer::CachedDataType type, const void* pData, size_t size)
		: m_refCnt(1)
		, m_lock(0)
		, m_type(type)
		, m_pData(pData)
		, m_pDataCopy(0)
		, m_size(size)
	{
		CryInterlockedIncrement(&ms_numInst);
	}

	~CCachedDataStore()
	{
		CryInterlockedDecrement(&ms_numInst);
		if (m_pDataCopy)
		{
			GFREE(m_pDataCopy);
			m_pDataCopy = 0;
		}
	}

private:
	static volatile int ms_numInst;

private:
	volatile int              m_refCnt;
	volatile LONG             m_lock;
	GRenderer::CachedDataType m_type;
	const void*               m_pData;
	void*                     m_pDataCopy;
	size_t                    m_size;
};

volatile int CCachedDataStore::ms_numInst = 0;

//////////////////////////////////////////////////////////////////////////

AllocateConstIntCVar(GRendererXRender, ms_sys_flash_debugdraw);
AllocateConstIntCVar(GRendererXRender, ms_sys_flash_newstencilclear);

float GRendererXRender::ms_sys_flash_stereo_maxparallax = 0.02f;
float GRendererXRender::ms_sys_flash_debugdraw_depth = 1.0f;

GRendererXRender::GRendererXRender()
	: m_pXRender(0)
	, m_stencilAvail(false)
	, m_renderMasked(false)
	, m_stencilCounter(0)
	, m_avoidStencilClear(false)
	, m_maskedRendering(false)
	, m_extendCanvasToVP(false)
	, m_scissorEnabled(false)
	, m_applyHalfPixelOffset(false)
	, m_maxVertices(0)
	, m_maxIndices(0)
	, m_matViewport()
	, m_matViewport3D()
	, m_matUncached()
	, m_matCached2D()
	, m_matCached3D()
	, m_matCachedWVP()
	, m_mat()
	, m_pMatWorld3D(0)
	, m_matView3D()
	, m_matProj3D()
	, m_matCached2DDirty(false)
	, m_matCached3DDirty(false)
	, m_matCachedWVPDirty(false)
	, m_cxform()
	, m_glyphVB()
	, m_blendOpStack()
	, m_curBlendMode(Blend_None)
	, m_rsIdx(0)
	, m_renderStats()
	, m_pDrawParams(0)
	, m_canvasRect(0, 0, 0, 0)
	, m_compDepth(0)
	, m_stereo3DiBaseDepth(0)
	, m_maxParallax(0)
	, m_customMaxParallax(-1.0f)
	, m_maxParallaxScene(0)
	, m_screenDepthScene(1.0f)
	, m_stereoFixedProjDepth(false)
	, m_isStereo(false)
	, m_isLeft(false)
	, m_mainThreadID(0)
	, m_renderThreadID(0)
	, m_pCmdBuf(0)
{
	m_pXRender = gEnv->pRenderer;
	assert(m_pXRender);

	m_stencilAvail = m_pXRender->GetStencilBpp() != 0;

	m_applyHalfPixelOffset = false;

	m_blendOpStack.reserve(16);

	m_renderStats[0].Clear();
	m_renderStats[1].Clear();

	m_pDrawParams = (SSF_GlobalDrawParams*) GALLOC(sizeof(SSF_GlobalDrawParams), GStat_Default_Mem);
	if (m_pDrawParams)
		new(m_pDrawParams) SSF_GlobalDrawParams;

	m_pXRender->SF_GetMeshMaxSize(m_maxVertices, m_maxIndices);
	m_pXRender->GetThreadIDs(m_mainThreadID, m_renderThreadID);
}

GRendererXRender::~GRendererXRender()
{
	if (m_pDrawParams)
	{
		m_pDrawParams->~SSF_GlobalDrawParams();
		GFREE(m_pDrawParams);
		m_pDrawParams = 0;
	}
}

bool GRendererXRender::GetRenderCaps(RenderCaps* pCaps)
{
	UInt32 nonPow2 = Cap_TexNonPower2;
	pCaps->CapBits = Cap_Index16 | Cap_FillGouraud | Cap_FillGouraudTex | Cap_CxformAdd | Cap_NestedMasks | nonPow2; // should we allow 32 bit indices if supported?
	#ifdef ENABLE_FLASH_FILTERS
	pCaps->CapBits |= Cap_RenderTargetPrePass;
	#endif
	pCaps->BlendModes = (1 << Blend_None) | (1 << Blend_Normal) | (1 << Blend_Multiply) | (1 << Blend_Lighten) | (1 << Blend_Darken) | (1 << Blend_Add) | (1 << Blend_Subtract);
	pCaps->VertexFormats = (1 << Vertex_None) | (1 << Vertex_XY16i) | (1 << Vertex_XY16iC32) | (1 << Vertex_XY16iCF32);
	pCaps->MaxTextureSize = m_pXRender->GetMaxTextureSize();
	return true;
}

GTexture* GRendererXRender::CreateTexture()
{
	return new GTextureXRender(this);
}

GTexture* GRendererXRender::CreateTextureYUV()
{
	return new GTextureXRenderYUV(this);
}

GRenderTarget* GRendererXRender::CreateRenderTarget()
{
	assert(0 && "GRendererXRender::CreateRenderTarget() -- Not implemented!");
	return 0;
}

void GRendererXRender::SetDisplayRenderTarget(GRenderTarget* /*pRT*/, bool /*setState*/)
{
	assert(0 && "GRendererXRender::SetDisplayRenderTarget() -- Not implemented!");
}

void GRendererXRender::PushRenderTarget(const GRectF& /*frameRect*/, GRenderTarget* /*pRT*/)
{
	assert(0 && "GRendererXRender::PushRenderTarget() -- Not implemented!");
}

void GRendererXRender::PopRenderTarget()
{
	#ifdef ENABLE_FLASH_FILTERS
	_RECORD_CMD_PREFIX
	  _RECORD_CMD(GRCBA_PopRenderTarget)
	_RECORD_CMD_POSTFIX(CMD_VOID_RETURN)

	int rtStackSize = m_renderTargetStack.size();

	//clear texture slot, may not be required
	ApplyTextureInfo(0);

	if (rtStackSize > 0)
	{
		//restore viewport matrix when popping render target
		RenderTargetStackElem& rtElem = m_renderTargetStack[rtStackSize - 1];
		m_matViewport = rtElem.oldViewMat;

		//set(0) causes resolve
		m_pXRender->SetRenderTarget(0);

		m_pXRender->SetViewport(0, 0, rtElem.oldViewportWidth, rtElem.oldViewportHeight);

		m_renderTargetStack.pop_back();
	}
	else
	{
		CryFatalError("PopRenderTarget called incorrectly");
	}
	#endif
}

static inline void matrixOrthoOffCenterLH(Matrix44& mat, f32 l, f32 r, f32 b, f32 t)
{
	mat.m00 = 2.0f / (r - l);
	mat.m01 = 0;
	mat.m02 = 0;
	mat.m03 = (l + r) / (l - r);
	mat.m10 = 0;
	mat.m11 = 2.0f / (t - b);
	mat.m12 = 0;
	mat.m13 = (t + b) / (b - t);
	mat.m20 = 0;
	mat.m21 = 0;
	mat.m22 = 1.0f;
	mat.m23 = 0.0f;
	mat.m30 = 0;
	mat.m31 = 0;
	mat.m32 = 0;
	mat.m33 = 1.0f;
}

GTexture* GRendererXRender::PushTempRenderTarget(const GRectF& frameRect, UInt targetW, UInt targetH, bool /*wantStencil = 0*/)
{
	#ifdef ENABLE_FLASH_FILTERS
	_RECORD_CMD_PREFIX
	GTextureXRenderTempRTLockless* pTempRT = new GTextureXRenderTempRTLockless(this);
	_RECORD_CMD(GRCBA_PushTempRenderTarget)
	_RECORD_CMD_ARG(frameRect)
	_RECORD_CMD_ARG(targetW)
	_RECORD_CMD_ARG(targetH)
	_RECORD_CMD_ARG(pTempRT)
	_RECORD_CMD_POSTFIX(pTempRT)

	UInt RTWidth = max((UInt)2, targetW);
	UInt RTHeight = max((UInt)2, targetH);

	//align to power of 2 dimensions
	if (!IsPowerOfTwo(RTWidth))
	{
		int log2 = IntegerLog2_RoundUp(RTWidth);
		RTWidth = 1 << log2;
	}

	if (!IsPowerOfTwo(RTHeight))
	{
		int log2 = IntegerLog2_RoundUp(RTHeight);
		RTHeight = 1 << log2;
	}

	int numActiveRTs = m_renderTargetStack.size();

	int numAllocatedRTs = m_renderTargets.size();
	GTextureXRender* pNewTempRT = NULL;

	//push pre allocated RT
	for (int i = 0; i < numAllocatedRTs; i++)
	{
		GTextureXRender* pCurTarget = m_renderTargets[i];
		int texID = pCurTarget->GetID();
		ITexture* pTex = m_pXRender->EF_GetTextureByID(texID);

		UInt curWidth = pTex->GetWidth();
		UInt curHeight = pTex->GetHeight();

		if (RTWidth <= curWidth && RTHeight <= curHeight && pCurTarget->GetRefCount() <= 1)
		{
			bool bNotInUse = true;

			for (uint32 j = 0; j < m_renderTargetStack.size(); j++)
			{
				if (pCurTarget == m_renderTargetStack[j].pRT)
				{
					bNotInUse = false;
					break;
				}
			}

			if (bNotInUse)
			{
				pNewTempRT = pCurTarget;
				break;
			}
		}
	}

	if (!pNewTempRT)
	{
		//allocate new RT
		int texID = m_pXRender->CreateRenderTarget(RTWidth, RTHeight, Clr_Transparent, eTF_R8G8B8A8);
		pNewTempRT = new GTextureXRenderTempRT(this, texID, eTF_R8G8B8A8);

		ITexture* pTex = m_pXRender->EF_GetTextureByID(texID);

		//prevent temp buffer stomping on back buffer
		pTex->SetRenderTargetTile(1);

		m_renderTargets.push_back(pNewTempRT);
	}

	RenderTargetStackElem newElem;

	newElem.pRT = pNewTempRT;
	newElem.oldViewMat = m_matViewport;

	int x, y, width, height;
	m_pXRender->GetViewport(&x, &y, &width, &height);

	newElem.oldViewportHeight = height;
	newElem.oldViewportWidth = width;

	m_renderTargetStack.push_back(newElem);
	m_pXRender->SetRenderTarget(pNewTempRT->GetID());

	//clear RT on push
	m_pXRender->ClearTargetsLater(FRT_CLEAR_COLOR);

	// set ViewportMatrix
	float dx = frameRect.Width();
	float dy = frameRect.Height();
	if (dx < 1) { dx = 1; }
	if (dy < 1) { dy = 1; }

	m_matViewport.SetIdentity();
	m_matViewport.m00 = 2.0f / dx;
	m_matViewport.m11 = -2.0f / dy;
	m_matViewport.m03 = -1.0f - m_matViewport.m00 * (frameRect.Left);
	m_matViewport.m13 = 1.0f - m_matViewport.m11 * (frameRect.Top);

	if (m_applyHalfPixelOffset)
	{
		// Adjust by -0.5 pixel to match DirectX pixel coverage rules.
		Float xhalfPixelAdjust = (targetW > 0) ? (1.0f / (Float)targetW) : 0.0f;
		Float yhalfPixelAdjust = (targetH > 0) ? (1.0f / (Float)targetH) : 0.0f;

		m_matViewport.m03 -= xhalfPixelAdjust;
		m_matViewport.m13 += yhalfPixelAdjust;
	}

	m_pXRender->SetViewport(0, 0, targetW, targetH);

	return pNewTempRT;
	#else
	return NULL;
	#endif
}

static inline void matrixOrthoToViewport(Matrix44& mat, int rtX0, int rtY0, int rtWidth, int rtHeight, int vpX0, int vpY0, int vpWidth, int vpHeight)
{
	//// map viewport into section of normalized clip space rect [(-1,1)...(1,-1)] using matrixOrthoOffCenterLH
	//float sx = 2.0f / (float) rtWidth;
	//float ox = (float) (rtX0 + rtX0 + rtWidth) / (float) (-rtWidth);

	//float sy = 2.0f / (float) -rtHeight;
	//float oy = (float) (rtY0 + rtY0 + rtHeight) / (float) (rtHeight);

	//float x0r = sx * vpX0 + ox;
	//float y0r = sy * vpY0 + oy;

	//float x1r = sx * (vpX0 + vpWidth ) + ox;
	//float y1r = sy * (vpY0 + vpHeight) + oy;

	//// remap normalized clip space rect [(-1,1)...(1,-1)] to viewport section in normalized clip space
	//float xs = (x1r - x0r) / 2.0f;
	//float xo = xs + x0r;

	//float ys = (y0r - y1r) / 2.0f;
	//float yo = ys + y1r;

	float xs = (float) vpWidth / (float) rtWidth;
	float ys = (float) vpHeight / (float) rtHeight;

	float xo = (vpWidth + 2.0f * (vpX0 - rtX0) - rtWidth) / (float) rtWidth;
	float yo = -(vpHeight + 2.0f * (vpY0 - rtY0) - rtHeight) / (float) rtHeight;

	mat.m00 = xs;
	mat.m01 = 0;
	mat.m02 = 0;
	mat.m03 = xo;
	mat.m10 = 0;
	mat.m11 = ys;
	mat.m12 = 0;
	mat.m13 = yo;
	mat.m20 = 0;
	mat.m21 = 0;
	mat.m22 = 1.0f;
	mat.m23 = 0.0f;
	mat.m30 = 0;
	mat.m31 = 0;
	mat.m32 = 0;
	mat.m33 = 1.0f;
}

void GRendererXRender::BeginDisplay(GColor backgroundColor, const GViewport& viewport, Float x0, Float x1, Float y0, Float y1)
{
	RECORD_CMD_6ARG(GRCBA_BeginDisplay, CMD_VOID_RETURN, backgroundColor, viewport, x0, x1, y0, y1);

	FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);

	UpdateStereoSettings();

	assert((x1 - x0) != 0 && (y1 - y0) != 0);

	float invWidth(1.0f / max((x1 - x0), 1.0f));
	float invHeight(1.0f / max((y1 - y0), 1.0f));

	int rtX0(0), rtY0(0), rtWidth(0), rtHeight(0);
	m_pXRender->GetViewport(&rtX0, &rtY0, &rtWidth, &rtHeight);
	rtWidth = max(rtWidth, 1);
	rtHeight = max(rtHeight, 1);

	Matrix44 matScale = Matrix44(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	matScale.m00 = (float) viewport.Width * invWidth;
	matScale.m11 = (float) viewport.Height * invHeight;
	matScale.m03 = -x0 * invWidth + (float) viewport.Left;
	matScale.m13 = -y0 * invHeight + (float) viewport.Top;

	Matrix44 matRescale;
	matrixOrthoOffCenterLH(matRescale, (f32) rtX0, (f32) (rtX0 + rtWidth), (f32) (rtY0 + rtHeight), (f32) rtY0);
	if (m_applyHalfPixelOffset)
	{
		matRescale.m03 -= 1.0f / (float) rtWidth;
		matRescale.m13 += 1.0f / (float) rtHeight;
	}
	m_matViewport = matRescale * matScale;

	matrixOrthoToViewport(m_matViewport3D, rtX0, rtY0, rtWidth, rtHeight, viewport.Left, viewport.Top, viewport.Width, viewport.Height);
	if (m_applyHalfPixelOffset)
	{
		m_matViewport3D.m03 -= 1.0f / (float) rtWidth;
		m_matViewport3D.m13 += 1.0f / (float) rtHeight;
	}

	#if !defined(_RELEASE)
	m_matViewport.m33 = ms_sys_flash_debugdraw_depth;
	m_matViewport3D.m33 = ms_sys_flash_debugdraw_depth;
	#endif

	uint32 nReverseDepthEnabled = 0;
	m_pXRender->EF_Query(EFQ_ReverseDepthEnabled, nReverseDepthEnabled);

	if (nReverseDepthEnabled)
	{
		Matrix44 matReverseDepth(IDENTITY);
		matReverseDepth.m22 = -1.0f;
		matReverseDepth.m23 = 1.0f;

		m_matViewport = matReverseDepth * m_matViewport;
		m_matViewport3D = matReverseDepth * m_matViewport3D;
	}

	m_pMatWorld3D = 0;

	m_scissorEnabled = (viewport.Flags & GViewport::View_UseScissorRect) != 0;
	if (m_scissorEnabled)
	{
		int scX0(rtX0), scY0(rtY0), scX1(rtX0 + rtWidth), scY1(rtY0 + rtHeight);
		if (scX0 < viewport.ScissorLeft)
			scX0 = viewport.ScissorLeft;
		if (scY0 < viewport.ScissorTop)
			scY0 = viewport.ScissorTop;
		if (scX1 > viewport.ScissorLeft + viewport.ScissorWidth)
			scX1 = viewport.ScissorLeft + viewport.ScissorWidth;
		if (scY1 > viewport.ScissorTop + viewport.ScissorHeight)
			scY1 = viewport.ScissorTop + viewport.ScissorHeight;

		m_pXRender->SetScissor(scX0, scY0, scX1 - scX0, scY1 - scY0);
	}

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.blendModeStates = 0;
	params.renderMaskedStates = 0;
	m_stencilCounter = 0;

	m_blendOpStack.resize(0);
	ApplyBlendMode(Blend_None);

	if (!m_extendCanvasToVP)
	{
		m_canvasRect = SRect(x0, y0, x1, y1);
	}
	else
	{
		const float x0vp = (-1.0f - m_matViewport.m03) / m_matViewport.m00;
		const float y0vp = (1.0f - m_matViewport.m13) / m_matViewport.m11;
		const float x1vp = (1.0f - m_matViewport.m03) / m_matViewport.m00;
		const float y1vp = (-1.0f - m_matViewport.m13) / m_matViewport.m11;
		m_canvasRect = SRect(x0vp, y0vp, x1vp, y1vp);
	}
	m_rsIdx = 0;
	m_pXRender->EF_Query(EFQ_RenderThreadList, m_rsIdx);
	assert(m_rsIdx < CRY_ARRAY_COUNT(m_renderStats));

	// Make parallax resolution independent
	m_maxParallaxScene *= (float) rtWidth / (float) viewport.Width;

	Clear(!ms_sys_flash_debugdraw ? backgroundColor : GColor(0, 255, 0, 255), m_canvasRect);
}

void GRendererXRender::EndDisplay()
{
	RECORD_CMD(GRCBA_EndDisplay, CMD_VOID_RETURN);

	assert(m_avoidStencilClear || !m_stencilCounter);

	if (m_scissorEnabled)
		m_pXRender->SetScissor();
}

void GRendererXRender::SetMatrix(const GMatrix2D& m)
{
	RECORD_CMD_1ARG(GRCBA_SetMatrix, CMD_VOID_RETURN, m);

	m_mat = m;
	m_matCached2DDirty = true;
	m_matCached3DDirty = true;
}

void GRendererXRender::SetUserMatrix(const GMatrix2D& /*m*/)
{
	assert(0 && "GRendererXRender::SetUserMatrix() -- Not implemented!");
}

void GRendererXRender::SetCxform(const Cxform& cx)
{
	RECORD_CMD_1ARG(GRCBA_SetCxform, CMD_VOID_RETURN, cx);

	m_cxform = cx;
}

void GRendererXRender::PushBlendMode(BlendType mode)
{
	RECORD_CMD_1ARG(GRCBA_PushBlendMode, CMD_VOID_RETURN, mode);

	m_blendOpStack.push_back(m_curBlendMode);

	if ((mode > Blend_Layer) && (m_curBlendMode != mode))
	{
		ApplyBlendMode(mode);
	}
}

void GRendererXRender::PopBlendMode()
{
	RECORD_CMD(GRCBA_PopBlendMode, CMD_VOID_RETURN);

	assert(m_blendOpStack.size() && "GRendererXRender::PopBlendMode() -- Blend mode stack is empty!");

	if (m_blendOpStack.size())
	{
		BlendType newBlendMode(Blend_None);

		for (int i(m_blendOpStack.size() - 1); i >= 0; --i)
		{
			if (m_blendOpStack[i] > Blend_Layer)
			{
				newBlendMode = m_blendOpStack[i];
				break;
			}
		}

		m_blendOpStack.pop_back();

		if (newBlendMode != m_curBlendMode)
		{
			ApplyBlendMode(newBlendMode);
		}
	}
}

void GRendererXRender::SetPerspective3D(const GMatrix3D& projMatIn)
{
	RECORD_CMD_1ARG(GRCBA_SetPerspective3D, CMD_VOID_RETURN, projMatIn);

	m_matProj3D = projMatIn;
	m_matCachedWVPDirty = true;
	m_matCached3DDirty = true;
}

void GRendererXRender::SetView3D(const GMatrix3D& viewMatIn)
{
	RECORD_CMD_1ARG(GRCBA_SetView3D, CMD_VOID_RETURN, viewMatIn);

	m_matView3D = viewMatIn;
	m_matCachedWVPDirty = true;
	m_matCached3DDirty = true;

	m_stereo3DiBaseDepth = -m_matView3D.M_[3][2];
	assert(m_stereo3DiBaseDepth >= 0);
}

void GRendererXRender::SetWorld3D(const GMatrix3D* pWorldMatIn)
{
	_RECORD_CMD_PREFIX
	if (pWorldMatIn)
	{
		_RECORD_CMD(GRCBA_SetWorld3D)
		_RECORD_CMD_ARG(*pWorldMatIn)
	}
	else
	{
		_RECORD_CMD(GRCBA_SetWorld3D_NullPtr)
	}
	_RECORD_CMD_POSTFIX(CMD_VOID_RETURN)

	m_pMatWorld3D = pWorldMatIn;
	m_matCachedWVPDirty = pWorldMatIn != 0;
	m_matCached3DDirty = pWorldMatIn != 0;
}

void GRendererXRender::ApplyBlendMode(BlendType blendMode)
{
	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;

	IF (ms_sys_flash_debugdraw, 0)
	{
		m_curBlendMode = Blend_None;
		params.blendModeStates = 0;
		return;
	}

	m_curBlendMode = blendMode;
	params.blendModeStates = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
	params.premultipliedAlpha = false;

	switch (blendMode)
	{
	case Blend_None:
	case Blend_Normal:
	case Blend_Layer:
	case Blend_Screen:
	case Blend_Difference:
	case Blend_Invert:
	case Blend_Overlay:
	case Blend_HardLight:
		params.blendModeStates = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
		break;
	case Blend_Multiply:
		params.blendModeStates = GS_BLSRC_DSTCOL | GS_BLDST_ZERO;
		break;
	case Blend_Add:
	case Blend_Subtract:
	case Blend_Lighten:
	case Blend_Darken:
		params.blendModeStates = GS_BLSRC_SRCALPHA | GS_BLDST_ONE;
		break;
	case Blend_Alpha:
	case Blend_Erase:
		params.blendModeStates = GS_BLSRC_ZERO | GS_BLDST_ONE;
		break;
	default:
		assert(!"GRendererXRender::ApplyBlendMode() -- Unsupported blend type!");
		break;
	}

	switch (blendMode)
	{
	case Blend_Subtract:
		params.blendOp = SSF_GlobalDrawParams::RevSubstract;
		break;
	case Blend_Lighten:
		params.blendOp = SSF_GlobalDrawParams::Max;
		break;
	case Blend_Darken:
		params.blendOp = SSF_GlobalDrawParams::Min;
		break;
	default:
		params.blendOp = SSF_GlobalDrawParams::Add;
		break;
	}
	params.isMultiplyDarkBlendMode = m_curBlendMode == Blend_Multiply || m_curBlendMode == Blend_Darken;

	if (m_maskedRendering && m_isStereo)
	{
		if (params.blendOp == SSF_GlobalDrawParams::Add && params.blendModeStates == (GS_BLSRC_SRCALPHA | GS_BLDST_ONE))
		{
			// Special blend mode for stereo, where icons get occluded by weapon
			params.blendModeStates = GS_BLSRC_DSTALPHA | GS_BLDST_ONE;
			params.premultipliedAlpha = true;
		}
	}
}

void GRendererXRender::ApplyColor(const GColor& src)
{
	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;

	ColorF& dst(params.colTransform1st);
	const float scale(1.0f / 255.0f);
	dst.r = src.GetRed() * scale;
	dst.g = src.GetGreen() * scale;
	dst.b = src.GetBlue() * scale;
	dst.a = src.GetAlpha() * scale;
	if (m_curBlendMode == Blend_Multiply || m_curBlendMode == Blend_Darken)
	{
		float a(dst.a);
		dst.r = 1.0f + a * (dst.r - 1.0f);
		dst.g = 1.0f + a * (dst.g - 1.0f);
		dst.b = 1.0f + a * (dst.b - 1.0f);
	}

	params.colTransform2nd = ColorF(0, 0, 0, 0);
}

void GRendererXRender::ApplyTextureInfo(unsigned int texSlot, const FillTexture* pFill)
{
	assert(texSlot < 2);
	IF (texSlot >= 2, 0)
		return;

	SSF_GlobalDrawParams::STextureInfo& texInfo(m_pDrawParams->texture[texSlot]);
	if (pFill && pFill->pTexture)
	{
		const GTextureXRenderBase* pTex(static_cast<const GTextureXRenderBase*>(pFill->pTexture));

		const GMatrix2D& m(pFill->TextureMatrix);

		assert(!pTex->IsYUV());
		texInfo.texID = pTex->GetID();

		texInfo.texGenMat.m00 = m.M_[0][0];
		texInfo.texGenMat.m01 = m.M_[0][1];
		texInfo.texGenMat.m02 = 0;
		texInfo.texGenMat.m03 = m.M_[0][2];

		texInfo.texGenMat.m10 = m.M_[1][0];
		texInfo.texGenMat.m11 = m.M_[1][1];
		texInfo.texGenMat.m12 = 0;
		texInfo.texGenMat.m13 = m.M_[1][2];

		texInfo.texGenMat.m20 = 0;
		texInfo.texGenMat.m21 = 0;
		texInfo.texGenMat.m22 = 0;
		texInfo.texGenMat.m23 = 1;

		texInfo.texState = (pFill->WrapMode == Wrap_Clamp) ? SSF_GlobalDrawParams::TS_Clamp : 0;
		texInfo.texState |= (pFill->SampleMode == Sample_Linear) ? SSF_GlobalDrawParams::TS_FilterLin : 0;
	}
	else
	{
		texInfo.texID = 0;
		texInfo.texGenMat.SetIdentity();
		texInfo.texState = 0;
	}
}

void GRendererXRender::ApplyCxform()
{
	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.colTransform1st = ColorF(m_cxform.M_[0][0], m_cxform.M_[1][0], m_cxform.M_[2][0], m_cxform.M_[3][0]);
	params.colTransform2nd = ColorF(m_cxform.M_[0][1] / 255.0f, m_cxform.M_[1][1] / 255.0f, m_cxform.M_[2][1] / 255.0f, m_cxform.M_[3][1] / 255.0f);
}

void GRendererXRender::CalcTransMat2D(const GMatrix2D* pSrc, Matrix44* __restrict pDst) const
{
	assert(pSrc && pDst);

	Matrix44 mat;
	mat.m00 = pSrc->M_[0][0];
	mat.m01 = pSrc->M_[0][1];
	mat.m02 = 0;
	mat.m03 = pSrc->M_[0][2];
	mat.m10 = pSrc->M_[1][0];
	mat.m11 = pSrc->M_[1][1];
	mat.m12 = 0;
	mat.m13 = pSrc->M_[1][2];
	mat.m20 = 0;
	mat.m21 = 0;
	mat.m22 = 0;
	mat.m23 = m_compDepth;
	mat.m30 = 0;
	mat.m31 = 0;
	mat.m32 = 0;
	mat.m33 = 1;

	*pDst = m_matViewport * mat;
}

void GRendererXRender::CalcTransMat3D(const GMatrix2D* pSrc, Matrix44* __restrict pDst)
{
	assert(pSrc && pDst);
	assert(m_pMatWorld3D);

	if (m_matCachedWVPDirty)
	{
		GMatrix3D matProjPatched = m_matProj3D;
		assert((matProjPatched.M_[2][3] == -1.0f || matProjPatched.M_[2][3] == 0.0f) && "GRendererXRender::SetPerspective3D() -- projMatIn is not right handed");
		if (!m_stereoFixedProjDepth)
		{
			matProjPatched.M_[2][0] = -m_maxParallax; // negative here as m_matProj3D is RH
			matProjPatched.M_[3][0] = -m_maxParallax * m_stereo3DiBaseDepth;
		}
		else
		{
			const float maxParallax = m_maxParallaxScene;

			const float projDepth = m_stereo3DiBaseDepth;
			const float screenDepth = m_screenDepthScene;
			const float perceivedDepth = max((*m_pMatWorld3D).M_[3][2], 1e-4f);

			matProjPatched.M_[3][0] += projDepth * maxParallax * (1.0f - screenDepth / perceivedDepth);
			matProjPatched.M_[2][3] = 0;
			matProjPatched.M_[3][3] = projDepth;
		}

		GMatrix3D mat = *m_pMatWorld3D;
		mat.Append(m_matView3D);
		mat.Append(matProjPatched);

		Matrix44 matCachedWVP;
		matCachedWVP.m00 = mat.M_[0][0];
		matCachedWVP.m01 = mat.M_[1][0];
		matCachedWVP.m02 = mat.M_[2][0];
		matCachedWVP.m03 = mat.M_[3][0];
		matCachedWVP.m10 = mat.M_[0][1];
		matCachedWVP.m11 = mat.M_[1][1];
		matCachedWVP.m12 = mat.M_[2][1];
		matCachedWVP.m13 = mat.M_[3][1];
		if (!m_stereoFixedProjDepth)
		{
			matCachedWVP.m20 = 0;
			matCachedWVP.m21 = 0;
			matCachedWVP.m22 = 0;
			matCachedWVP.m23 = 0;                                                                   // force set homogeneous z to 0
		}
		else
		{
			const float d = m_compDepth;
			matCachedWVP.m20 = mat.M_[0][3] * d;
			matCachedWVP.m21 = mat.M_[1][3] * d;
			matCachedWVP.m22 = mat.M_[2][3] * d;
			matCachedWVP.m23 = mat.M_[3][3] * d;
		}
		matCachedWVP.m30 = mat.M_[0][3];
		matCachedWVP.m31 = mat.M_[1][3];
		matCachedWVP.m32 = mat.M_[2][3];
		matCachedWVP.m33 = mat.M_[3][3];

		m_matCachedWVP = m_matViewport3D * matCachedWVP;
		m_matCachedWVPDirty = false;
	}

	Matrix44 mat;
	mat.m00 = pSrc->M_[0][0];
	mat.m01 = pSrc->M_[0][1];
	mat.m02 = 0;
	mat.m03 = pSrc->M_[0][2];
	mat.m10 = pSrc->M_[1][0];
	mat.m11 = pSrc->M_[1][1];
	mat.m12 = 0;
	mat.m13 = pSrc->M_[1][2];
	mat.m20 = 0;
	mat.m21 = 0;
	mat.m22 = 1;
	mat.m23 = 0;
	mat.m30 = 0;
	mat.m31 = 0;
	mat.m32 = 0;
	mat.m33 = 1;

	*pDst = m_matCachedWVP * mat;
}

void GRendererXRender::ApplyMatrix(const GMatrix2D* pMatIn)
{
	assert(pMatIn);

	bool is3D = m_pMatWorld3D != 0;
	bool useCaching = pMatIn == &m_mat;

	Matrix44* __restrict pMatOut = 0;

	if (!is3D)
	{
		pMatOut = useCaching ? &m_matCached2D : &m_matUncached;
		if (!useCaching || m_matCached2DDirty)
		{
			CalcTransMat2D(pMatIn, pMatOut);
			if (useCaching)
				m_matCached2DDirty = false;
		}
	}
	else
	{
		pMatOut = useCaching ? &m_matCached3D : &m_matUncached;
		if (!useCaching || m_matCached3DDirty)
		{
			CalcTransMat3D(pMatIn, pMatOut);
			if (useCaching)
				m_matCached3DDirty = false;
		}
	}

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.pTransMat = pMatOut;
}

static inline size_t VertexSize(GRenderer::VertexFormat vf)
{
	switch (vf)
	{
	case GRenderer::Vertex_XY16i:
		return 4;
	case GRenderer::Vertex_XY32f:
		return 8;
	case GRenderer::Vertex_XY16iC32:
		return 8;
	case GRenderer::Vertex_XY16iCF32:
		return 12;
	case GRenderer::Vertex_None:
		return 0;

	default:
		assert(0);
		return 0;
	}
}

void GRendererXRender::SetVertexData(const void* pVertices, int numVertices, VertexFormat vf, CacheProvider* pCache)
{
	_RECORD_CMD_PREFIX
	  _RECORD_CMD(GRCBA_SetVertexData)
	const size_t dataSize = numVertices * VertexSize(vf);
	CCachedDataStore* pStore = CCachedDataStore::Create(this, pCache, Cached_Vertex, pVertices, dataSize);
	_RECORD_CMD_ARG(pStore)
	IF (!pStore, 0)
	{
		_RECORD_CMD_DATA(pVertices, dataSize)
	}
	_RECORD_CMD_ARG(numVertices)
	_RECORD_CMD_ARG(vf)
	_RECORD_CMD_POSTFIX(CMD_VOID_RETURN)

	assert((!pVertices || vf == Vertex_XY16i || vf == Vertex_XY16iC32 || vf == Vertex_XY16iCF32) && "GRendererXRender::SetVertexData() -- Unsupported source vertex format!");
	if (pVertices && numVertices > 0 && numVertices <= m_maxVertices && (vf == Vertex_XY16i || vf == Vertex_XY16iC32 || vf == Vertex_XY16iCF32))
	{
		SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
		switch (vf)
		{
		case Vertex_XY16i:
			params.vertexFmt = SSF_GlobalDrawParams::Vertex_XY16i;
			break;
		case Vertex_XY16iC32:
			params.vertexFmt = SSF_GlobalDrawParams::Vertex_XY16iC32;
			break;
		case Vertex_XY16iCF32:
			params.vertexFmt = SSF_GlobalDrawParams::Vertex_XY16iCF32;
			break;
		}
		params.pVertexPtr = pVertices;
		params.numVertices = numVertices;
	}
	else
	{
		SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
		params.vertexFmt = SSF_GlobalDrawParams::Vertex_None;
		params.pVertexPtr = 0;
		params.numVertices = 0;
	#if !defined(_RELEASE)
		if (numVertices > m_maxVertices)
			CryGFxLog::GetAccess().LogWarning("Trying to send too many vertices at once (amount = %d, limit = %d)!", numVertices, m_maxVertices);
	#endif
	}
}

static inline size_t IndexSize(GRenderer::IndexFormat idxf)
{
	switch (idxf)
	{
	case GRenderer::Index_16:
		return 2;
	case GRenderer::Index_32:
		return 4;
	case GRenderer::Index_None:
		return 0;

	default:
		assert(0);
		return 0;
	}
}

void GRendererXRender::SetIndexData(const void* pIndices, int numIndices, IndexFormat idxf, CacheProvider* pCache)
{
	_RECORD_CMD_PREFIX
	  _RECORD_CMD(GRCBA_SetIndexData)
	const size_t dataSize = numIndices * IndexSize(idxf);
	CCachedDataStore* pStore = CCachedDataStore::Create(this, pCache, Cached_Index, pIndices, dataSize);
	_RECORD_CMD_ARG(pStore)
	IF (!pStore, 0)
	{
		_RECORD_CMD_DATA(pIndices, dataSize)
	}
	_RECORD_CMD_ARG(numIndices)
	_RECORD_CMD_ARG(idxf)
	_RECORD_CMD_POSTFIX(CMD_VOID_RETURN)

	assert((!pIndices || idxf == Index_16) && "GRendererXRender::SetIndexData() -- Unsupported source index format!");
	if (pIndices && numIndices > 0 && numIndices <= m_maxIndices && idxf == Index_16)
	{
		SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
		params.indexFmt = SSF_GlobalDrawParams::Index_16;
		params.pIndexPtr = pIndices;
		params.numIndices = numIndices;
	}
	else
	{
		SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
		params.indexFmt = SSF_GlobalDrawParams::Index_None;
		params.pIndexPtr = 0;
		params.numIndices = 0;
	#if !defined(_RELEASE)
		if (numIndices > m_maxIndices)
			CryGFxLog::GetAccess().LogWarning("Trying to send too many indices at once (amount = %d, limit = %d)!", numIndices, m_maxIndices);
	#endif
	}
}

void GRendererXRender::ReleaseCachedData(CachedData* pData, CachedDataType /*type*/)
{
	//assert(IsMainThread()); // see notes in CCachedDataStore::BackupAndRelease()
	CCachedDataStore* pStore = (CCachedDataStore*) (pData ? pData->GetRendererData() : 0);
	if (pStore)
		pStore->BackupAndRelease();
}

void GRendererXRender::DrawIndexedTriList(int baseVertexIndex, int minVertexIndex, int numVertices, int startIndex, int triangleCount)
{
	RECORD_CMD_5ARG(GRCBA_DrawIndexedTriList, CMD_VOID_RETURN, baseVertexIndex, minVertexIndex, numVertices, startIndex, triangleCount);

	FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	IF ((m_renderMasked && !m_stencilAvail) || params.vertexFmt == SSF_GlobalDrawParams::Vertex_None || params.indexFmt == SSF_GlobalDrawParams::Index_None, 0)
		return;

	assert(params.vertexFmt != SSF_GlobalDrawParams::Vertex_None);
	assert(params.indexFmt == SSF_GlobalDrawParams::Index_16);

	// setup render parameters
	ApplyMatrix(&m_mat);

	// draw triangle list
	m_pXRender->SF_DrawIndexedTriList(baseVertexIndex, minVertexIndex, numVertices, startIndex, triangleCount, params);

	params.pVertexPtr = 0;
	params.numVertices = 0;
	params.pIndexPtr = 0;
	params.numIndices = 0;

	// update stats
	m_renderStats[m_rsIdx].Triangles += triangleCount;
	++m_renderStats[m_rsIdx].Primitives;
}

void GRendererXRender::DrawLineStrip(int baseVertexIndex, int lineCount)
{
	RECORD_CMD_2ARG(GRCBA_DrawLineStrip, CMD_VOID_RETURN, baseVertexIndex, lineCount);

	FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	IF ((m_renderMasked && !m_stencilAvail) || params.vertexFmt != SSF_GlobalDrawParams::Vertex_XY16i, 0)
		return;

	// setup render parameters
	ApplyMatrix(&m_mat);

	// draw triangle list
	if (params.numVertices)
		m_pXRender->SF_DrawLineStrip(baseVertexIndex, lineCount, params);

	params.pVertexPtr = 0;
	params.numVertices = 0;

	// update stats
	m_renderStats[m_rsIdx].Lines += lineCount;
	++m_renderStats[m_rsIdx].Primitives;
}

void GRendererXRender::LineStyleDisable()
{
}

void GRendererXRender::LineStyleColor(GColor color)
{
	RECORD_CMD_1ARG(GRCBA_LineStyleColor, CMD_VOID_RETURN, color);

	FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);

	ApplyTextureInfo(0);
	ApplyTextureInfo(1);
	ApplyColor(m_cxform.Transform(color));

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.fillType = SSF_GlobalDrawParams::SolidColor;
}

void GRendererXRender::FillStyleDisable()
{
}

void GRendererXRender::FillStyleColor(GColor color)
{
	RECORD_CMD_1ARG(GRCBA_FillStyleColor, CMD_VOID_RETURN, color);

	FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);

	ApplyTextureInfo(0);
	ApplyTextureInfo(1);
	ApplyColor(m_cxform.Transform(color));

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.fillType = SSF_GlobalDrawParams::SolidColor;
}

void GRendererXRender::FillStyleBitmap(const FillTexture* pFill)
{
	_RECORD_CMD_PREFIX
	  _RECORD_CMD(GRCBA_FillStyleBitmap)
	_RECORD_CMD_ARG_FILLTEX(pFill)
	_RECORD_CMD_POSTFIX(CMD_VOID_RETURN)

	FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);

	ApplyTextureInfo(0, pFill);
	ApplyTextureInfo(1);
	ApplyCxform();

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.fillType = SSF_GlobalDrawParams::Texture;
}

void GRendererXRender::FillStyleGouraud(GouraudFillType fillType, const FillTexture* pTexture0, const FillTexture* pTexture1, const FillTexture* pTexture2)
{
	_RECORD_CMD_PREFIX
	  _RECORD_CMD(GRCBA_FillStyleGouraud)
	_RECORD_CMD_ARG(fillType)
	_RECORD_CMD_ARG_FILLTEX(pTexture0)
	_RECORD_CMD_ARG_FILLTEX(pTexture1)
	_RECORD_CMD_ARG_FILLTEX(pTexture2)
	_RECORD_CMD_POSTFIX(CMD_VOID_RETURN)

	FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);

	const FillTexture* t0(0);
	const FillTexture* t1(0);

	if (pTexture0 || pTexture1 || pTexture2)
	{
		t0 = pTexture0;
		if (!t0)
			t0 = pTexture1;
		if (pTexture1)
			t1 = pTexture1;
	}

	ApplyTextureInfo(0, t0);
	ApplyTextureInfo(1, t1);
	ApplyCxform();

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	switch (fillType)
	{
	case GFill_Color:
		params.fillType = SSF_GlobalDrawParams::GColor;
		break;
	case GFill_1Texture:
		params.fillType = SSF_GlobalDrawParams::G1Texture;
		break;
	case GFill_1TextureColor:
		params.fillType = SSF_GlobalDrawParams::G1TextureColor;
		break;
	case GFill_2Texture:
		params.fillType = SSF_GlobalDrawParams::G2Texture;
		break;
	case GFill_2TextureColor:
		params.fillType = SSF_GlobalDrawParams::G2TextureColor;
		break;
	case GFill_3Texture:
		params.fillType = SSF_GlobalDrawParams::G3Texture;
		break;
	default:
		assert(0);
		break;
	}
}

void GRendererXRender::DrawBitmaps(BitmapDesc* pBitmapList, int listSize, int startIndex, int count, const GTexture* pTi, const GMatrix2D& m, CacheProvider* pCache)
{
	_RECORD_CMD_PREFIX
	_RECORD_CMD(GRCBA_DrawBitmaps)
	if (pBitmapList && listSize > 0)
	{
		const size_t dataSize = listSize * sizeof(BitmapDesc);
		CCachedDataStore* pStore = CCachedDataStore::Create(this, pCache, Cached_BitmapList, pBitmapList, dataSize);
		_RECORD_CMD_ARG(pStore)
		IF (!pStore, 0)
		{
			_RECORD_CMD_DATA(pBitmapList, dataSize)
		}
	}
	else
	{
		_RECORD_CMD_ARG((CCachedDataStore*) 0)
		_RECORD_CMD_DATA(0, 0)
	}
	_RECORD_CMD_ARG(listSize)
	_RECORD_CMD_ARG(startIndex)
	_RECORD_CMD_ARG(count)
	_RECORD_CMD_ARG(pTi) GTextureAddRef(const_cast<GTexture*>(pTi));
	_RECORD_CMD_ARG(m)
	_RECORD_CMD_POSTFIX(CMD_VOID_RETURN)

	FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);

	IF (!pBitmapList || !pTi, 0)
		return;

	// resize buffer
	uint32 numVertices(4 * count + (count - 1) * 2);
	m_glyphVB.resize(numVertices);

	// merge multiple bitmaps into one draw call via stitching the triangle strip
	for (int bitmapIdx(0), vertOffs(0); bitmapIdx < count; ++bitmapIdx)
	{
		const BitmapDesc& bitmapDesc(pBitmapList[bitmapIdx + startIndex]);
		const Rect& coords(bitmapDesc.Coords);
		const Rect& uvCoords(bitmapDesc.TextureCoords);

		const GColor& srcCol(bitmapDesc.Color);
		unsigned int color((srcCol.GetAlpha() << 24) | (srcCol.GetBlue() << 16) | (srcCol.GetGreen() << 8) | srcCol.GetRed());
		SGlyphVertex& v0(m_glyphVB[vertOffs]);
		v0.x = coords.Left;
		v0.y = coords.Top;
		v0.u = uvCoords.Left;
		v0.v = uvCoords.Top;
		v0.col = color;
		++vertOffs;

		if (bitmapIdx > 0)
		{
			m_glyphVB[vertOffs] = m_glyphVB[vertOffs - 1];
			++vertOffs;
		}

		SGlyphVertex& v1(m_glyphVB[vertOffs]);
		v1.x = coords.Right;
		v1.y = coords.Top;
		v1.u = uvCoords.Right;
		v1.v = uvCoords.Top;
		v1.col = color;
		++vertOffs;

		SGlyphVertex& v2(m_glyphVB[vertOffs]);
		v2.x = coords.Left;
		v2.y = coords.Bottom;
		v2.u = uvCoords.Left;
		v2.v = uvCoords.Bottom;
		v2.col = color;
		++vertOffs;

		SGlyphVertex& v3(m_glyphVB[vertOffs]);
		v3.x = coords.Right;
		v3.y = coords.Bottom;
		v3.u = uvCoords.Right;
		v3.v = uvCoords.Bottom;
		v3.col = color;
		++vertOffs;

		if (bitmapIdx < count - 1)
		{
			m_glyphVB[vertOffs] = m_glyphVB[vertOffs - 1];
			++vertOffs;
		}
	}

	// setup render parameters
	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;

	ApplyMatrix(&m);

	const GTextureXRenderBase* pTex(static_cast<const GTextureXRenderBase*>(pTi));

	if (!pTex->IsYUV())
	{
		params.texID_YUVA[0] = params.texID_YUVA[1] = params.texID_YUVA[2] = params.texID_YUVA[3] = 0;
		params.texture[0].texID = pTex ? pTex->GetID() : 0;
		params.texture[0].texGenMat.SetIdentity();
		params.texture[0].texState = SSF_GlobalDrawParams::TS_FilterTriLin | SSF_GlobalDrawParams::TS_Clamp;
		params.fillType = pTex->GetFmt() != eTF_A8 ? SSF_GlobalDrawParams::GlyphTexture : SSF_GlobalDrawParams::GlyphAlphaTexture;
		params.texGenYUVAStereo = Vec2(1.0f, 0.0f);
	}
	else
	{
		const GTextureXRenderYUV* pTexYUV(static_cast<const GTextureXRenderYUV*>(pTi));
		const int32 numIDs = pTexYUV->GetNumIDs();
		assert(numIDs == 3 || numIDs == 4);
		const int32* pTexIDs = pTexYUV->GetIDs();
		assert(pTexIDs);
		params.texID_YUVA[0] = pTexIDs[0];
		params.texID_YUVA[1] = pTexIDs[1];
		params.texID_YUVA[2] = pTexIDs[2];
		params.texID_YUVA[3] = numIDs ? pTexIDs[3] : 0;
		params.texture[0].texID = 0;
		params.texture[0].texGenMat.SetIdentity();
		params.texture[0].texState = SSF_GlobalDrawParams::TS_FilterLin | SSF_GlobalDrawParams::TS_Clamp;
		params.fillType = numIDs == 3 ? SSF_GlobalDrawParams::GlyphTextureYUV : SSF_GlobalDrawParams::GlyphTextureYUVA;

		Vec2 texGenYUVAStereo(0.0f, 0.0f);
		if (pTexYUV->IsStereoContent())
		{
			if (m_isStereo)
				texGenYUVAStereo = Vec2(m_isLeft ? 2.0f / 3.0f : 1.0f / 3.0f, m_isLeft ? 0.0f : 2.0f / 3.0f);
			else
				texGenYUVAStereo = Vec2(2.0f / 3.0f, 0.0f);
		}
		else
			texGenYUVAStereo = Vec2(1.0f, 0.0f);
		params.texGenYUVAStereo = texGenYUVAStereo;
	}

	ApplyTextureInfo(1);
	ApplyCxform();

	params.vertexFmt = SSF_GlobalDrawParams::Vertex_Glyph;
	params.pVertexPtr = &m_glyphVB[0];
	params.numVertices = numVertices;

	// draw glyphs
	m_pXRender->SF_DrawGlyphClear(params);

	params.pVertexPtr = 0;
	params.numVertices = 0;

	// update stats
	m_renderStats[m_rsIdx].Triangles += numVertices - 2;
	++m_renderStats[m_rsIdx].Primitives;
}

void GRendererXRender::BeginSubmitMask(SubmitMaskMode maskMode)
{
	RECORD_CMD_1ARG(GRCBA_BeginSubmitMask, CMD_VOID_RETURN, maskMode);

	FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);

	m_renderMasked = true;
	IF (!m_stencilAvail || ms_sys_flash_debugdraw == 2, 0)
		return;

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.renderMaskedStates = GS_STENCIL | GS_COLMASK_NONE;

	switch (maskMode)
	{
	case Mask_Clear:
		{
			if (!m_avoidStencilClear)
			{
				if (!ms_sys_flash_newstencilclear)
					m_pXRender->ClearTargetsLater(FRT_CLEAR_STENCIL);
				else
				{
					m_pXRender->SF_ConfigMask(IRenderer::BeginSubmitMask_Clear, 0);
					Clear(GColor(0, 0, 0, 255), m_canvasRect);
				}
				m_pXRender->SF_ConfigMask(IRenderer::BeginSubmitMask_Clear, 1);
				m_stencilCounter = 1;
			}
			else
			{
				++m_stencilCounter;
				m_pXRender->SF_ConfigMask(IRenderer::BeginSubmitMask_Clear, m_stencilCounter);
			}
			break;
		}
	case Mask_Increment:
		{
			m_pXRender->SF_ConfigMask(IRenderer::BeginSubmitMask_Inc, m_stencilCounter);
			++m_stencilCounter;
			break;
		}
	case Mask_Decrement:
		{
			m_pXRender->SF_ConfigMask(IRenderer::BeginSubmitMask_Dec, m_stencilCounter);
			--m_stencilCounter;
			break;
		}
	}
	++m_renderStats[m_rsIdx].Masks;
}

void GRendererXRender::EndSubmitMask()
{
	RECORD_CMD(GRCBA_EndSubmitMask, CMD_VOID_RETURN);

	FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);

	m_renderMasked = true;
	IF (!m_stencilAvail || ms_sys_flash_debugdraw == 2, 0)
		return;

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.renderMaskedStates = GS_STENCIL;

	m_pXRender->SF_ConfigMask(IRenderer::EndSubmitMask, m_stencilCounter);
}

void GRendererXRender::DisableMask()
{
	RECORD_CMD(GRCBA_DisableMask, CMD_VOID_RETURN);

	FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);

	m_renderMasked = false;
	IF (!m_stencilAvail || ms_sys_flash_debugdraw == 2, 0)
		return;

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.renderMaskedStates = 0;

	m_pXRender->SF_ConfigMask(IRenderer::DisableMask, 0);

	if (!m_avoidStencilClear)
		m_stencilCounter = 0;
}

UInt GRendererXRender::CheckFilterSupport(const BlurFilterParams& params)
{
	#ifdef ENABLE_FLASH_FILTERS
	UInt flags = FilterSupport_Ok;
	if (params.Passes > 1 || (params.BlurX * params.BlurY > 32))
		flags |= FilterSupport_Multipass;

	return flags;
	#else
	return FilterSupport_None;
	#endif
}

void GRendererXRender::DrawBlurRect(GTexture* pSrcIn, const GRectF& inSrcRect, const GRectF& inDestRect, const BlurFilterParams& params, bool islast)
{
	#ifdef ENABLE_FLASH_FILTERS
	_RECORD_CMD_PREFIX
	  _RECORD_CMD(GRCBA_DrawBlurRect)
	_RECORD_CMD_ARG(pSrcIn) GTextureAddRef(const_cast<GTexture*>(pSrcIn));
	_RECORD_CMD_ARG(inSrcRect)
	_RECORD_CMD_ARG(inDestRect)
	_RECORD_CMD_ARG(params)
	_RECORD_CMD_ARG(islast)
	_RECORD_CMD_POSTFIX(CMD_VOID_RETURN)

	//FIXME 3d matrix screws up blur, should this be necessary?
	const GMatrix3D * pMat3D_Cached = m_pMatWorld3D;
	SetWorld3D(NULL);

	GPtr<GTextureXRender> psrc = (GTextureXRender*)pSrcIn;
	GPtr<GTextureXRender> pnextsrc = NULL;
	GRectF srcrect, destrect(-1, -1, 1, 1);

	UInt n = params.Passes;

	BlurFilterParams pass[3];
	SSF_GlobalDrawParams::EBlurType passis[3];

	pass[0] = params;
	pass[1] = params;
	pass[2] = params;

	bool mul = (m_curBlendMode == Blend_Multiply || m_curBlendMode == Blend_Darken);
	passis[0] = passis[1] = SSF_GlobalDrawParams::Box2Blur;
	passis[2] = mul ? SSF_GlobalDrawParams::Box2BlurMul : SSF_GlobalDrawParams::Box2Blur;

	m_pDrawParams->blendOp = SSF_GlobalDrawParams::Add;
	m_pDrawParams->vertexFmt = SSF_GlobalDrawParams::Vertex_Glyph;

	if (params.Mode & Filter_Shadow)
	{
		if (params.Mode & Filter_HideObject)
		{
			passis[2] = SSF_GlobalDrawParams::Box2Shadowonly;

			if (params.Mode & Filter_Highlight)
				passis[2] = (SSF_GlobalDrawParams::EBlurType) (passis[2] + SSF_GlobalDrawParams::shadows_Highlight);
		}
		else
		{
			if (params.Mode & Filter_Inner)
				passis[2] = SSF_GlobalDrawParams::Box2InnerShadow;
			else
				passis[2] = SSF_GlobalDrawParams::Box2Shadow;

			if (params.Mode & Filter_Knockout)
				passis[2] = (SSF_GlobalDrawParams::EBlurType) (passis[2] + SSF_GlobalDrawParams::shadows_Knockout);

			if (params.Mode & Filter_Highlight)
				passis[2] = (SSF_GlobalDrawParams::EBlurType) (passis[2] + SSF_GlobalDrawParams::shadows_Highlight);
		}

		if (mul)
			passis[2] = (SSF_GlobalDrawParams::EBlurType) (passis[2] + SSF_GlobalDrawParams::shadows_Mul);
	}

	if (params.BlurX * params.BlurY > 32)
	{
		n *= 2;
		pass[0].BlurY = 1;
		pass[1].BlurX = 1;
		pass[2].BlurX = 1;

		passis[0] = passis[1] = SSF_GlobalDrawParams::Box1Blur;
		if (passis[2] == SSF_GlobalDrawParams::Box2Blur)
			passis[2] = SSF_GlobalDrawParams::Box1Blur;
		else if (passis[2] == SSF_GlobalDrawParams::Box2BlurMul)
			passis[2] = SSF_GlobalDrawParams::Box1BlurMul;
	}

	UInt bufWidth = (UInt)ceilf(inSrcRect.Width());
	UInt bufHeight = (UInt)ceilf(inSrcRect.Height());
	UInt last = n - 1;

	for (UInt i = 0; i < n; i++)
	{
		UInt passi = (i == last) ? 2 : (i & 1);
		const BlurFilterParams& pparams = pass[passi];
		SSF_GlobalDrawParams::BlurFilterParams& sf_filterParams = m_pDrawParams->blurParams;
		sf_filterParams.blurType = passis[passi];

		if (i != n - 1)
		{
			pnextsrc = (GTextureXRender*)PushTempRenderTarget(GRectF(-1, -1, 1, 1), bufWidth, bufHeight);

			ApplyMatrix(&GMatrix2D::Identity);
			destrect = GRectF(-1, -1, 1, 1);
		}
		else
		{
			pnextsrc = NULL;
			ApplyMatrix(&m_mat);
			destrect = inDestRect;
		}

		ITexture* pSrcITex = m_pXRender->EF_GetTextureByID(psrc->GetID());
		int texWidth = pSrcITex->GetWidth();
		int texHeight = pSrcITex->GetHeight();

		srcrect = GRectF(inSrcRect.Left * 1.0f / texWidth, inSrcRect.Top * 1.0f / texHeight,
		                 inSrcRect.Right * 1.0f / texWidth, inSrcRect.Bottom * 1.0f / texHeight);

		// not sure why this must set always to Add, but if set to current blendmode in last pass it will darken the results...
		//    if (i < last)
		{
			m_pDrawParams->blendModeStates = GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCALPHA;
			m_pDrawParams->blendOp = SSF_GlobalDrawParams::Add;
		}
		//    else
		//    {
		//      ApplyBlendMode(m_curBlendMode);
		//    }

		//set blend state
		const float mult = 1.0f / 255.0f;

		//if (FShaderUniforms[passis[passi]][FSU_cxadd] >= 0)
		{
			if (i == n - 1)
			{
				float cxformData[4 * 2] =
				{
					params.cxform.M_[0][0] * params.cxform.M_[3][0],
					params.cxform.M_[1][0] * params.cxform.M_[3][0],
					params.cxform.M_[2][0] * params.cxform.M_[3][0],
					params.cxform.M_[3][0],

					params.cxform.M_[0][1] * mult * params.cxform.M_[3][0],
					params.cxform.M_[1][1] * mult * params.cxform.M_[3][0],
					params.cxform.M_[2][1] * mult * params.cxform.M_[3][0],
					params.cxform.M_[3][1] * mult
				};

				m_pDrawParams->colTransform1st = ColorF(cxformData[0], cxformData[1], cxformData[2], cxformData[3]);
				m_pDrawParams->colTransform2nd = ColorF(cxformData[4], cxformData[5], cxformData[6], cxformData[7]);
			}
			else
			{
				m_pDrawParams->colTransform1st = ColorF(1, 1, 1, 1);
				m_pDrawParams->colTransform2nd = ColorF(0, 0, 0, 0);
			}
		}

		float SizeX = uint(pparams.BlurX - 1) * 0.5f;
		float SizeY = uint(pparams.BlurY - 1) * 0.5f;

		if (passis[passi] == SSF_GlobalDrawParams::Box1Blur || passis[passi] == SSF_GlobalDrawParams::Box1BlurMul)
		{
			const float fsize[] = { pparams.BlurX > 1 ? SizeX : SizeY, 0, 0, 1.0f / ((SizeX * 2 + 1) * (SizeY * 2 + 1)) };

			sf_filterParams.blurFilterSize = Vec4(fsize[0], fsize[1], fsize[2], fsize[3]);
			sf_filterParams.blurFilterScale = Vec2(pparams.BlurX > 1 ? 1.0f / texWidth : 0.f, pparams.BlurY > 1 ? 1.0f / texHeight : 0.f);
		}
		else
		{
			const float fsize[] = { SizeX, SizeY, 0, 1.0f / ((SizeX * 2 + 1) * (SizeY * 2 + 1)) };

			sf_filterParams.blurFilterSize = Vec4(fsize[0], fsize[1], fsize[2], fsize[3]);
			sf_filterParams.blurFilterScale = Vec2(1.0f / texWidth, 1.0f / texHeight);
		}

		//    if (FShaderUniforms[passis[passi]][FSU_offset] >= 0)
		{
			sf_filterParams.blurFilterOffset = Vec2(-params.Offset.x, -params.Offset.y);
		}

		//    if (FShaderUniforms[passis[passi]][FSU_scolor] >= 0)
		{
			sf_filterParams.blurFilterColor1 = ColorF(params.Color.GetRed() * mult, params.Color.GetGreen() * mult, params.Color.GetBlue() * mult, params.Color.GetAlpha() * mult);
			//      if (FShaderUniforms[passis[passi]][FSU_scolor2] >= 0)
			sf_filterParams.blurFilterColor2 = ColorF(params.Color2.GetRed() * mult, params.Color2.GetGreen() * mult, params.Color2.GetBlue() * mult, params.Color2.GetAlpha() * mult);
		}

		FillTexture fillTexture;
		fillTexture.pTexture = psrc;
		fillTexture.WrapMode = Wrap_Clamp;
		fillTexture.SampleMode = Sample_Linear;

		FillTexture sourceTexture;
		sourceTexture.pTexture = pSrcIn;
		sourceTexture.WrapMode = Wrap_Clamp;
		sourceTexture.SampleMode = Sample_Linear;

		ApplyTextureInfo(0, &fillTexture);
		ApplyTextureInfo(1, &sourceTexture);

		uint32 col = 0;
		SGlyphVertex strip[4] =
		{
			{ destrect.Left,  destrect.Top,    srcrect.Left,  srcrect.Top,    col },
			{ destrect.Right, destrect.Top,    srcrect.Right, srcrect.Top,    col },
			{ destrect.Left,  destrect.Bottom, srcrect.Left,  srcrect.Bottom, col },
			{ destrect.Right, destrect.Bottom, srcrect.Right, srcrect.Bottom, col }
		};

		m_pDrawParams->pVertexPtr = (void*)strip;
		m_pDrawParams->numVertices = 4;

		m_pXRender->SF_DrawBlurRect(m_pDrawParams);

		if (i != n - 1)
		{
			PopRenderTarget();
			psrc = pnextsrc;
		}
	}

	SetWorld3D(pMat3D_Cached);
	m_renderStats[m_rsIdx].Filters += n;

	//restore blend mode
	ApplyBlendMode(m_curBlendMode);
	#endif
}

void GRendererXRender::DrawColorMatrixRect(GTexture* pSrcIn, const GRectF& inSrcRect, const GRectF& inDestRect, const Float* pMatrix, bool islast)
{
	#ifdef ENABLE_FLASH_FILTERS
	_RECORD_CMD_PREFIX
	  _RECORD_CMD(GRCBA_DrawColorMatrixRect)
	_RECORD_CMD_ARG(pSrcIn) GTextureAddRef(const_cast<GTexture*>(pSrcIn));
	_RECORD_CMD_ARG(inSrcRect)
	_RECORD_CMD_ARG(inDestRect)
	_RECORD_CMD_DATA(pMatrix, 5 * 4 * sizeof(Float))
	_RECORD_CMD_ARG(islast)
	_RECORD_CMD_POSTFIX(CMD_VOID_RETURN)

	FillTexture fillTexture;
	fillTexture.pTexture = pSrcIn;
	fillTexture.WrapMode = Wrap_Clamp;
	fillTexture.SampleMode = Sample_Linear;
	ApplyTextureInfo(0, &fillTexture);

	m_pDrawParams->colTransformMat[0] = ColorF(pMatrix[0], pMatrix[1], pMatrix[2], pMatrix[3]);
	m_pDrawParams->colTransformMat[1] = ColorF(pMatrix[4], pMatrix[5], pMatrix[6], pMatrix[7]);
	m_pDrawParams->colTransformMat[2] = ColorF(pMatrix[8], pMatrix[9], pMatrix[10], pMatrix[11]);
	m_pDrawParams->colTransformMat[3] = ColorF(pMatrix[12], pMatrix[13], pMatrix[14], pMatrix[15]);
	m_pDrawParams->colTransform2nd = ColorF(pMatrix[16], pMatrix[17], pMatrix[18], pMatrix[19]);

	m_pDrawParams->blendModeStates = GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCALPHA;
	m_pDrawParams->blendOp = SSF_GlobalDrawParams::Add;

	if (m_curBlendMode == Blend_Multiply || m_curBlendMode == Blend_Darken)
		m_pDrawParams->fillType = SSF_GlobalDrawParams::GlyphTextureMatMul;
	else
		m_pDrawParams->fillType = SSF_GlobalDrawParams::GlyphTextureMat;

	GTextureXRender* psrc = (GTextureXRender*)pSrcIn;
	ITexture* pSrcITex = m_pXRender->EF_GetTextureByID(psrc->GetID());
	int texWidth = pSrcITex->GetWidth();
	int texHeight = pSrcITex->GetHeight();

	GRectF srcrect = GRectF(inSrcRect.Left * 1.0f / texWidth, inSrcRect.Top * 1.0f / texHeight,
	                        inSrcRect.Right * 1.0f / texWidth, inSrcRect.Bottom * 1.0f / texHeight);

	ApplyMatrix(&m_mat);

	m_pDrawParams->vertexFmt = SSF_GlobalDrawParams::Vertex_Glyph;
	m_pDrawParams->blendOp = SSF_GlobalDrawParams::Add;

	uint32 col = 0xffffffff;
	SGlyphVertex strip[4] =
	{
		{ inDestRect.Left,  inDestRect.Top,    srcrect.Left,  srcrect.Top,    col },
		{ inDestRect.Right, inDestRect.Top,    srcrect.Right, srcrect.Top,    col },
		{ inDestRect.Left,  inDestRect.Bottom, srcrect.Left,  srcrect.Bottom, col },
		{ inDestRect.Right, inDestRect.Bottom, srcrect.Right, srcrect.Bottom, col }
	};

	m_pDrawParams->pVertexPtr = (void*)strip;
	m_pDrawParams->numVertices = 4;

	m_pXRender->SF_DrawGlyphClear(*m_pDrawParams);

	//restore blend mode
	ApplyBlendMode(m_curBlendMode);
	#endif
}

void GRendererXRender::GetRenderStats(Stats* pStats, bool resetStats)
{
	threadID rsIdx = 0;
	m_pXRender->EF_Query(IsMainThread() ? EFQ_MainThreadList : EFQ_RenderThreadList, rsIdx);
	assert(rsIdx < CRY_ARRAY_COUNT(m_renderStats));

	if (pStats)
		memcpy(pStats, &m_renderStats[rsIdx], sizeof(Stats));
	if (resetStats)
		m_renderStats[rsIdx].Clear();
}

void GRendererXRender::GetStats(GStatBag* /*pBag*/, bool /*reset*/)
{
	assert(0 && "GRendererXRender::GRendererXRender::GetStats() -- Not implemented!");
}

void GRendererXRender::ReleaseResources()
{
	const int nTempRenderTargets = m_renderTargets.size();

	for (int i = 0; i < nTempRenderTargets; ++i)
	{
		m_pXRender->DestroyRenderTarget(m_renderTargets[i]->GetID());
		SAFE_RELEASE(m_renderTargets[i]);
	}

	m_renderTargets.clear();
	m_renderTargetStack.clear();
}

IRenderer* GRendererXRender::GetXRender() const
{
	return m_pXRender;
}

namespace GRendererXRenderClearInternal
{
const float safeRangeScale = 8.0f;

struct MapToSafeRange
{
	MapToSafeRange(float v) : val(v) {}
	operator int16() const
	{
		float v = val / safeRangeScale;
		return (int16) (v >= 0 ? v + 0.5f : v - 0.5f);
	}
	float val;
};
}

void GRendererXRender::Clear(const GColor& backgroundColor, const SRect& rect)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);

	if (backgroundColor.GetAlpha() == 0)
		return;

	using namespace GRendererXRenderClearInternal;

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;

	// setup quad
	int16 quad[4 * 2] =
	{
		MapToSafeRange(rect.m_x0), MapToSafeRange(rect.m_y0),
		MapToSafeRange(rect.m_x1), MapToSafeRange(rect.m_y0),
		MapToSafeRange(rect.m_x0), MapToSafeRange(rect.m_y1),
		MapToSafeRange(rect.m_x1), MapToSafeRange(rect.m_y1)
	};

	// setup render parameters
	ApplyTextureInfo(0);
	ApplyTextureInfo(1);
	ApplyColor(backgroundColor);

	Matrix44 mat(m_matViewport);
	mat.m00 *= safeRangeScale;
	mat.m11 *= safeRangeScale;

	params.pTransMat = &mat;
	params.fillType = SSF_GlobalDrawParams::SolidColor;
	params.vertexFmt = SSF_GlobalDrawParams::Vertex_XY16i;
	params.pVertexPtr = &quad[0];
	params.numVertices = 4;

	int oldBendState(params.blendModeStates);
	params.blendModeStates = backgroundColor.GetAlpha() < 255 ? GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA : 0;
	int oldRenderMaskedStates(params.renderMaskedStates);
	params.renderMaskedStates = m_renderMasked ? params.renderMaskedStates : 0;

	// draw quad
	m_pXRender->SF_DrawGlyphClear(params);

	params.pVertexPtr = 0;
	params.numVertices = 0;
	params.blendModeStates = oldBendState;
	params.renderMaskedStates = oldRenderMaskedStates;

	// update stats
	m_renderStats[m_rsIdx].Triangles += 2;
	++m_renderStats[m_rsIdx].Primitives;
}

void GRendererXRender::SetCompositingDepth(float depth)
{
	m_compDepth = (depth >= 0 && depth <= 1) ? depth : 0;
}

void GRendererXRender::SetStereoMode(bool stereo, bool isLeft)
{
	m_isStereo = stereo;
	m_isLeft = isLeft;
}

void GRendererXRender::UpdateStereoSettings()
{
	IStereoRenderer* pStereoRenderer = m_pXRender->GetIStereoRenderer();
	assert(pStereoRenderer);

	const float maxParallax = m_customMaxParallax >= 0.0f ? m_customMaxParallax : ms_sys_flash_stereo_maxparallax;
	m_maxParallax = !m_isStereo ? 0 : m_isLeft ? -maxParallax : maxParallax;
	m_maxParallax *= pStereoRenderer->GetStereoStrength();

	const float maxParallaxScene = pStereoRenderer->GetMaxSeparationScene() * 2;
	m_maxParallaxScene = !m_isStereo ? 0 : m_isLeft ? -maxParallaxScene : maxParallaxScene;
	m_screenDepthScene = pStereoRenderer->GetZeroParallaxPlaneDist();
}

void GRendererXRender::StereoEnforceFixedProjectionDepth(bool enforce)
{
	m_stereoFixedProjDepth = enforce;
}

void GRendererXRender::StereoSetCustomMaxParallax(float maxParallax)
{
	m_customMaxParallax = maxParallax;
}

void GRendererXRender::AvoidStencilClear(bool avoid)
{
	m_avoidStencilClear = avoid;
}

void GRendererXRender::EnableMaskedRendering(bool enable)
{
	m_maskedRendering = enable;
}

void GRendererXRender::ExtendCanvasToViewport(bool extend)
{
	m_extendCanvasToVP = extend;
}

void GRendererXRender::SetThreadIDs(uint32 mainThreadID, uint32 renderThreadID)
{
	m_mainThreadID = mainThreadID;
	m_renderThreadID = renderThreadID;
	assert(IsMainThread());
}

void GRendererXRender::SetRecordingCommandBuffer(GRendererCommandBuffer* pCmdBuf)
{
	assert(IsMainThread());
	m_pCmdBuf = pCmdBuf;
}

void GRendererXRender::InitCVars()
{
	#if defined(_DEBUG)
	{
		static bool s_init = false;
		assert(!s_init);
		s_init = true;
	}
	#endif

	DefineConstIntCVar3("sys_flash_debugdraw", ms_sys_flash_debugdraw, sys_flash_debugdraw_DefVal, VF_CHEAT | VF_CHEAT_NOCHECK, "Enables debug drawing of Flash assets (1). Canvas area is drawn in bright green.\nAlso draw masks (2).");
	DefineConstIntCVar3("sys_flash_newstencilclear", ms_sys_flash_newstencilclear, sys_flash_newstencilclear_DefVal, VF_NULL, "Clears stencil buffer for mask rendering by drawing rects (1) as opposed to issuing clear commands (0)");
	REGISTER_CVAR2("sys_flash_stereo_maxparallax", &ms_sys_flash_stereo_maxparallax, ms_sys_flash_stereo_maxparallax, VF_NULL, "Maximum parallax when viewing Flash content in stereo 3D");
	#if !defined(_RELEASE)
	REGISTER_CVAR2("sys_flash_debugdraw_depth", &ms_sys_flash_debugdraw_depth, ms_sys_flash_debugdraw_depth, VF_CHEAT | VF_CHEAT_NOCHECK, "Projects Flash assets to a certain depth to be able to zoom in/out (default is 1.0)");
	#endif
}

void GRendererCommandBuffer::Render(GRendererXRender* pRenderer, bool releaseResources)
{
	Playback(pRenderer, releaseResources);
}

class GRendererNULL : public GRenderer
{
	// GRenderer interface
public:
	virtual bool           GetRenderCaps(RenderCaps*)                                                                    { return false; }
	virtual GTexture*      CreateTexture()                                                                               { return 0; }
	virtual GTexture*      CreateTextureYUV()                                                                            { return 0; }

	virtual GRenderTarget* CreateRenderTarget()                                                                          { return 0; }
	virtual void           SetDisplayRenderTarget(GRenderTarget*, bool)                                                  {}
	virtual void           PushRenderTarget(const GRectF&, GRenderTarget*)                                               {}
	virtual void           PopRenderTarget()                                                                             {}
	virtual GTexture*      PushTempRenderTarget(const GRectF&, UInt, UInt, bool)                                         { return 0; }

	virtual void           BeginDisplay(GColor, const GViewport&, Float, Float, Float, Float)                            {}
	virtual void           EndDisplay()                                                                                  {}

	virtual void           SetMatrix(const GMatrix2D&)                                                                   {}
	virtual void           SetUserMatrix(const GMatrix2D&)                                                               {}
	virtual void           SetCxform(const Cxform&)                                                                      {}

	virtual void           PushBlendMode(BlendType)                                                                      {}
	virtual void           PopBlendMode()                                                                                {}

	virtual void           SetPerspective3D(const GMatrix3D&)                                                            {}
	virtual void           SetView3D(const GMatrix3D&)                                                                   {}
	virtual void           SetWorld3D(const GMatrix3D*)                                                                  {}

	virtual void           SetVertexData(const void*, int, VertexFormat, CacheProvider*)                                 {}
	virtual void           SetIndexData(const void*, int, IndexFormat, CacheProvider*)                                   {}
	virtual void           ReleaseCachedData(CachedData*, CachedDataType)                                                {}

	virtual void           DrawIndexedTriList(int, int, int, int, int)                                                   {}
	virtual void           DrawLineStrip(int, int)                                                                       {}

	virtual void           LineStyleDisable()                                                                            {}
	virtual void           LineStyleColor(GColor color)                                                                  {}

	virtual void           FillStyleDisable()                                                                            {}
	virtual void           FillStyleColor(GColor)                                                                        {}
	virtual void           FillStyleBitmap(const FillTexture*)                                                           {}
	virtual void           FillStyleGouraud(GouraudFillType, const FillTexture*, const FillTexture*, const FillTexture*) {}

	virtual void           DrawBitmaps(BitmapDesc*, int, int, int, const GTexture*, const GMatrix2D&, CacheProvider*)    {}

	virtual void           BeginSubmitMask(SubmitMaskMode)                                                               {}
	virtual void           EndSubmitMask()                                                                               {}
	virtual void           DisableMask()                                                                                 {}

	virtual UInt           CheckFilterSupport(const BlurFilterParams&)                                                   { return FilterSupport_None; }
	virtual void           DrawBlurRect(GTexture*, const GRectF&, const GRectF&, const BlurFilterParams&, bool)          {}
	virtual void           DrawColorMatrixRect(GTexture*, const GRectF&, const GRectF&, const Float*, bool)              {}

	virtual void           GetRenderStats(Stats*, bool)                                                                  {}
	virtual void           GetStats(GStatBag*, bool)                                                                     {}
	virtual void           ReleaseResources()                                                                            {}
};

void GRendererCommandBuffer::DropResourceRefs()
{
	GRendererNULL nullRenderer;
	Playback(&nullRenderer, true);
}

static inline void UnlockAndRelease(CCachedDataStore* pStore, const bool release)
{
	if (pStore)
	{
		pStore->Unlock();
		if (release)
			pStore->Release();
	}
}

static inline void ReleaseFillStyleBitmapTex(GTexture* pTex, const bool release)
{
	if (release)
		GTextureRelease(pTex);
}

static inline void ReleaseFillStyleGouraudTex(GTexture* pTex0, GTexture* pTex1, GTexture* pTex2, const bool release)
{
	if (release)
	{
		GTextureRelease(pTex0);
		GTextureRelease(pTex1);
		GTextureRelease(pTex2);
	}
}

template<class T>
void GRendererCommandBuffer::Playback(T* pRenderer, const bool releaseResources)
{
	assert(pRenderer);

	CCachedDataStore* pLastVertexStore = 0;
	CCachedDataStore* pLastIndexStore = 0;

	GTexture* pLastFillStyleBitmapTex = 0;
	GTexture* pLastFillStyleGouraudTex0 = 0;
	GTexture* pLastFillStyleGouraudTex1 = 0;
	GTexture* pLastFillStyleGouraudTex2 = 0;

	const size_t cmdBufSize = Size();
	if (cmdBufSize)
	{
		ResetReadPos();
		while (GetReadPos() < cmdBufSize)
		{
			const EGRendererCommandBufferCmds curCmd = Read<EGRendererCommandBufferCmds>();
			_PLAYBACK_CMD_SUFFIX
			switch (curCmd)
			{
			case GRCBA_PopRenderTarget:
				{
					pRenderer->T::PopRenderTarget();
					break;
				}

			case GRCBA_PushTempRenderTarget:
				{
					const GRectF& frameRect = ReadRef<GRectF>();
					UInt targetW = Read<UInt>();
					UInt targetH = Read<UInt>();
					GTextureXRenderTempRTLockless* pRT = Read<GTextureXRenderTempRTLockless*>();
					pRT->SetRT(pRenderer->T::PushTempRenderTarget(frameRect, targetW, targetH, false));

					if (releaseResources)
						GTextureRelease(pRT);

					break;
				}

			case GRCBA_BeginDisplay:
				{
					GColor backgroundColor = Read<GColor>();
					const GViewport& viewport = ReadRef<GViewport>();
					Float x0 = Read<Float>();
					Float x1 = Read<Float>();
					Float y0 = Read<Float>();
					Float y1 = Read<Float>();
					pRenderer->T::BeginDisplay(backgroundColor, viewport, x0, x1, y0, y1);
					break;
				}
			case GRCBA_EndDisplay:
				{
					pRenderer->T::EndDisplay();
					break;
				}
			case GRCBA_SetMatrix:
				{
					const GMatrix2D& m = ReadRef<GMatrix2D>();
					pRenderer->T::SetMatrix(m);
					break;
				}
			case GRCBA_SetUserMatrix:
				{
					assert(0);
					break;
				}
			case GRCBA_SetCxform:
				{
					const GRenderer::Cxform& cx = ReadRef<GRenderer::Cxform>();
					pRenderer->T::SetCxform(cx);
					break;
				}
			case GRCBA_PushBlendMode:
				{
					GRenderer::BlendType mode = Read<GRenderer::BlendType>();
					pRenderer->T::PushBlendMode(mode);
					break;
				}
			case GRCBA_PopBlendMode:
				{
					pRenderer->T::PopBlendMode();
					break;
				}
			case GRCBA_SetPerspective3D:
				{
					const GMatrix3D& projMatIn = ReadRef<GMatrix3D>();
					pRenderer->T::SetPerspective3D(projMatIn);
					break;
				}
			case GRCBA_SetView3D:
				{
					const GMatrix3D& viewMatIn = ReadRef<GMatrix3D>();
					pRenderer->T::SetView3D(viewMatIn);
					break;
				}
			case GRCBA_SetWorld3D:
				{
					const GMatrix3D& worldMatIn = ReadRef<GMatrix3D>();
					pRenderer->T::SetWorld3D(&worldMatIn);
					break;
				}
			case GRCBA_SetWorld3D_NullPtr:
				{
					pRenderer->T::SetWorld3D(0);
					break;
				}
			case GRCBA_SetVertexData:
				{
					UnlockAndRelease(pLastVertexStore, releaseResources);
					CCachedDataStore* pStore = Read<CCachedDataStore*>();
					const void* pVertices = 0;
					if (pStore)
					{
						pStore->Lock();
						pVertices = pStore->GetPtr();
					}
					else
					{
						size_t size = Read<size_t>();
						pVertices = size ? GetReadPosPtr() : 0;
						Skip(size, true);
					}
					pLastVertexStore = pStore;
					const int numVertices = Read<int>();
					const GRenderer::VertexFormat vf = Read<GRenderer::VertexFormat>();
					pRenderer->T::SetVertexData(pVertices, numVertices, vf, 0);
					break;
				}
			case GRCBA_SetIndexData:
				{
					UnlockAndRelease(pLastIndexStore, releaseResources);
					CCachedDataStore* pStore = Read<CCachedDataStore*>();
					const void* pIndices = 0;
					if (pStore)
					{
						pStore->Lock();
						pIndices = pStore->GetPtr();
					}
					else
					{
						size_t size = Read<size_t>();
						pIndices = size ? GetReadPosPtr() : 0;
						Skip(size, true);
					}
					pLastIndexStore = pStore;
					const int numIndices = Read<int>();
					const GRenderer::IndexFormat idxf = Read<GRenderer::IndexFormat>();
					pRenderer->T::SetIndexData(pIndices, numIndices, idxf, 0);
					break;
				}
			case GRCBA_DrawIndexedTriList:
				{
					int baseVertexIndex = Read<int>();
					int minVertexIndex = Read<int>();
					int numVertices = Read<int>();
					int startIndex = Read<int>();
					int triangleCount = Read<int>();
					pRenderer->T::DrawIndexedTriList(baseVertexIndex, minVertexIndex, numVertices, startIndex, triangleCount);
					UnlockAndRelease(pLastVertexStore, releaseResources);
					pLastVertexStore = 0;
					UnlockAndRelease(pLastIndexStore, releaseResources);
					pLastIndexStore = 0;
					break;
				}
			case GRCBA_DrawLineStrip:
				{
					int baseVertexIndex = Read<int>();
					int lineCount = Read<int>();
					pRenderer->T::DrawLineStrip(baseVertexIndex, lineCount);
					UnlockAndRelease(pLastVertexStore, releaseResources);
					pLastVertexStore = 0;
					break;
				}
			case GRCBA_LineStyleDisable:
				{
					pRenderer->T::LineStyleDisable();
					break;
				}
			case GRCBA_LineStyleColor:
				{
					GColor color = Read<GColor>();
					pRenderer->T::LineStyleColor(color);
					break;
				}
			case GRCBA_FillStyleDisable:
				{
					pRenderer->T::FillStyleDisable();
					break;
				}
			case GRCBA_FillStyleColor:
				{
					GColor color = Read<GColor>();
					pRenderer->T::FillStyleColor(color);
					break;
				}
			case GRCBA_FillStyleBitmap:
				{
					ReleaseFillStyleBitmapTex(pLastFillStyleBitmapTex, releaseResources);
					const GRenderer::FillTexture& fill = ReadRef<GRenderer::FillTexture>();
					pRenderer->T::FillStyleBitmap(&fill);
					pLastFillStyleBitmapTex = fill.pTexture;
					break;
				}
			case GRCBA_FillStyleGouraud:
				{
					ReleaseFillStyleGouraudTex(pLastFillStyleGouraudTex0, pLastFillStyleGouraudTex1, pLastFillStyleGouraudTex2, releaseResources);
					GRenderer::GouraudFillType fillType = Read<GRenderer::GouraudFillType>();
					const GRenderer::FillTexture& texture0 = ReadRef<GRenderer::FillTexture>();
					const GRenderer::FillTexture& texture1 = ReadRef<GRenderer::FillTexture>();
					const GRenderer::FillTexture& texture2 = ReadRef<GRenderer::FillTexture>();
					pRenderer->T::FillStyleGouraud(fillType, &texture0, &texture1, &texture2);
					pLastFillStyleGouraudTex0 = texture0.pTexture;
					pLastFillStyleGouraudTex1 = texture1.pTexture;
					pLastFillStyleGouraudTex2 = texture2.pTexture;
					break;
				}
			case GRCBA_DrawBitmaps:
				{
					CCachedDataStore* pStore = Read<CCachedDataStore*>();
					GRenderer::BitmapDesc* pBitmapList = 0;
					if (pStore)
					{
						pStore->Lock();
						pBitmapList = (GRenderer::BitmapDesc*) pStore->GetPtr();
					}
					else
					{
						size_t size = Read<size_t>();
						pBitmapList = (GRenderer::BitmapDesc*) (size ? GetReadPosPtr() : 0);
						Skip(size, true);
					}
					int listSize = Read<int>();
					int startIndex = Read<int>();
					int count = Read<int>();
					const GTexture* pTi = Read<const GTexture*>();
					const GMatrix2D& m = ReadRef<GMatrix2D>();
					pRenderer->T::DrawBitmaps(pBitmapList, listSize, startIndex, count, pTi, m, 0);
					if (releaseResources)
						GTextureRelease(const_cast<GTexture*>(pTi));
					UnlockAndRelease(pStore, releaseResources);
					break;
				}
			case GRCBA_BeginSubmitMask:
				{
					GRenderer::SubmitMaskMode maskMode = Read<GRenderer::SubmitMaskMode>();
					pRenderer->T::BeginSubmitMask(maskMode);
					break;
				}
			case GRCBA_EndSubmitMask:
				{
					pRenderer->T::EndSubmitMask();
					break;
				}
			case GRCBA_DisableMask:
				{
					pRenderer->T::DisableMask();
					break;
				}

			case GRCBA_DrawBlurRect:
				{
					GTexture* pSrcIn = Read<GTexture*>();
					const GRectF& inSrcRect = ReadRef<GRectF>();
					const GRectF& inDestRect = ReadRef<GRectF>();
					const GRenderer::BlurFilterParams& params = ReadRef<GRenderer::BlurFilterParams>();
					const bool isLast = Read<bool>();

					pRenderer->T::DrawBlurRect(pSrcIn, inSrcRect, inDestRect, params, isLast);

					if (releaseResources)
						GTextureRelease(const_cast<GTexture*>(pSrcIn));

					break;
				}

			case GRCBA_DrawColorMatrixRect:
				{
					GTexture* pSrcIn = Read<GTexture*>();
					const GRectF& inSrcRect = ReadRef<GRectF>();
					const GRectF& inDestRect = ReadRef<GRectF>();
					size_t size = Read<size_t>();
					const Float* pMatrix = (Float*)(size ? GetReadPosPtr() : 0);
					Skip(size, true);
					const bool isLast = Read<bool>();

					pRenderer->T::DrawColorMatrixRect(pSrcIn, inSrcRect, inDestRect, pMatrix, isLast);

					if (releaseResources)
						GTextureRelease(const_cast<GTexture*>(pSrcIn));

					break;
				}

			default:
				{
					assert(0);
					break;
				}
			}
			_PLAYBACK_CMD_POSTFIX
		}
		assert(GetReadPos() == cmdBufSize);
	}

	UnlockAndRelease(pLastVertexStore, releaseResources);
	UnlockAndRelease(pLastIndexStore, releaseResources);

	ReleaseFillStyleBitmapTex(pLastFillStyleBitmapTex, releaseResources);
	ReleaseFillStyleGouraudTex(pLastFillStyleGouraudTex0, pLastFillStyleGouraudTex1, pLastFillStyleGouraudTex2, releaseResources);
};

#endif // #ifdef INCLUDE_SCALEFORM_SDK
