// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef _GRENDERER_XRENDER_H_
#define _GRENDERER_XRENDER_H_

#pragma once

#ifdef INCLUDE_SCALEFORM_SDK

	#pragma warning(push)
	#pragma warning(disable : 6326)// Potential comparison of a constant with another constant
	#pragma warning(disable : 6011)// Dereferencing NULL pointer
	#include <CryCore/Platform/CryWindows.h>
	#include <GRenderer.h> // includes <windows.h>
	#pragma warning(pop)
	#include <CryMath/Cry_Math.h>

	#include <vector>
	#include "GMemorySTLAlloc.h"

struct IRenderer;
class ITexture;
class GTextureXRender;
class GRendererXRender;
struct SSF_GlobalDrawParams;
struct GRendererCommandBuffer;

class GRendererXRender : public GRenderer
{
	// GRenderer interface
public:

	virtual bool           GetRenderCaps(RenderCaps* pCaps);
	virtual GTexture*      CreateTexture();
	virtual GTexture*      CreateTextureYUV();

	virtual GRenderTarget* CreateRenderTarget();
	virtual void           SetDisplayRenderTarget(GRenderTarget* pRT, bool setState = 1);
	virtual void           PushRenderTarget(const GRectF& frameRect, GRenderTarget* pRT);
	virtual void           PopRenderTarget();
	virtual GTexture*      PushTempRenderTarget(const GRectF& frameRect, UInt targetW, UInt targetH, bool wantStencil = 0);

	virtual void           BeginDisplay(GColor backgroundColor, const GViewport& viewport, Float x0, Float x1, Float y0, Float y1);
	virtual void           EndDisplay();

	virtual void           SetMatrix(const GMatrix2D& m);
	virtual void           SetUserMatrix(const GMatrix2D& m);
	virtual void           SetCxform(const Cxform& cx);

	virtual void           PushBlendMode(BlendType mode);
	virtual void           PopBlendMode();

	virtual void           SetPerspective3D(const GMatrix3D& projMatIn);
	virtual void           SetView3D(const GMatrix3D& viewMatIn);
	virtual void           SetWorld3D(const GMatrix3D* pWorldMatIn);

	virtual void           SetVertexData(const void* pVertices, int numVertices, VertexFormat vf, CacheProvider* pCache = 0);
	virtual void           SetIndexData(const void* pIndices, int numIndices, IndexFormat idxf, CacheProvider* pCache = 0);
	virtual void           ReleaseCachedData(CachedData* pData, CachedDataType type);

	virtual void           DrawIndexedTriList(int baseVertexIndex, int minVertexIndex, int numVertices, int startIndex, int triangleCount);
	virtual void           DrawLineStrip(int baseVertexIndex, int lineCount);

	virtual void           LineStyleDisable();
	virtual void           LineStyleColor(GColor color);

	virtual void           FillStyleDisable();
	virtual void           FillStyleColor(GColor color);
	virtual void           FillStyleBitmap(const FillTexture* pFill);
	virtual void           FillStyleGouraud(GouraudFillType fillType, const FillTexture* pTexture0 = 0, const FillTexture* pTexture1 = 0, const FillTexture* pTexture2 = 0);

	virtual void           DrawBitmaps(BitmapDesc* pBitmapList, int listSize, int startIndex, int count, const GTexture* pTi, const GMatrix2D& m, CacheProvider* pCache = 0);

	virtual void           BeginSubmitMask(SubmitMaskMode maskMode);
	virtual void           EndSubmitMask();
	virtual void           DisableMask();

	virtual UInt           CheckFilterSupport(const BlurFilterParams& params);
	virtual void           DrawBlurRect(GTexture* pSrcIn, const GRectF& inSrcRect, const GRectF& inDestRect, const BlurFilterParams& params, bool isLast = false);
	virtual void           DrawColorMatrixRect(GTexture* pSrcIn, const GRectF& inSrcRect, const GRectF& inDestRect, const Float* pMatrix, bool isLast = false);

	virtual void           GetRenderStats(Stats* pStats, bool resetStats = false);
	virtual void           GetStats(GStatBag* pBag, bool reset = true);
	virtual void           ReleaseResources();

public:
	static void InitCVars();

public:
	GRendererXRender();
	virtual ~GRendererXRender();
	IRenderer* GetXRender() const;

	void       SetCompositingDepth(float depth);

	void       SetStereoMode(bool stereo, bool isLeft);
	void       StereoEnforceFixedProjectionDepth(bool enforce);
	void       StereoSetCustomMaxParallax(float maxParallax);

	void       AvoidStencilClear(bool avoid);
	void       EnableMaskedRendering(bool enable);
	void       ExtendCanvasToViewport(bool extend);

	void       SetThreadIDs(uint32 mainThreadID, uint32 renderThreadID);
	void       SetRecordingCommandBuffer(GRendererCommandBuffer* pCmdBuf);
	bool       IsMainThread() const   { uint32 threadID = GetCurrentThreadId(); return threadID == m_mainThreadID; }
	bool       IsRenderThread() const { uint32 threadID = GetCurrentThreadId(); return threadID == m_renderThreadID; }

	void       GetMemoryUsage(ICrySizer* pSizer) const
	{
		// !!! don't have to add anything, all allocations go through GFx memory interface !!!
	}

private:
	struct SRect
	{
		SRect(float x0, float y0, float x1, float y1) : m_x0(x0), m_y0(y0), m_x1(x1), m_y1(y1) {}
		float m_x0, m_y0, m_x1, m_y1;
	};

private:
	void Clear(const GColor& backgroundColor, const SRect& rect);

	void CalcTransMat2D(const GMatrix2D* pSrc, Matrix44* __restrict pDst) const;
	void CalcTransMat3D(const GMatrix2D* pSrc, Matrix44* __restrict pDst);

	void ApplyBlendMode(BlendType blendMode);
	void ApplyColor(const GColor& src);
	void ApplyTextureInfo(unsigned int texSlot, const FillTexture* pFill = 0);
	void ApplyCxform();
	void ApplyMatrix(const GMatrix2D* pMatIn);

	void UpdateStereoSettings();

public:
	const std::vector<GTextureXRender*, GMemorySTLAlloc<GTextureXRender*>>& GetTempRenderTargets() const { return m_renderTargets; }

private:
	enum
	{
		sys_flash_debugdraw_DefVal       = 0,
		sys_flash_newstencilclear_DefVal = 1
	};

	DeclareStaticConstIntCVar(ms_sys_flash_debugdraw, sys_flash_debugdraw_DefVal);
	DeclareStaticConstIntCVar(ms_sys_flash_newstencilclear, sys_flash_newstencilclear_DefVal);

	static float ms_sys_flash_stereo_maxparallax;
	static float ms_sys_flash_debugdraw_depth;

private:
	IRenderer* m_pXRender;

	bool       m_stencilAvail;
	bool       m_renderMasked;
	int        m_stencilCounter;
	bool       m_avoidStencilClear;
	bool       m_maskedRendering;
	bool       m_extendCanvasToVP;

	bool       m_scissorEnabled;
	bool       m_applyHalfPixelOffset;

	int        m_maxVertices;
	int        m_maxIndices;

	// transformation matrices
	Matrix44         m_matViewport;
	Matrix44         m_matViewport3D;
	Matrix44         m_matUncached;
	Matrix44         m_matCached2D;
	Matrix44         m_matCached3D;
	Matrix44         m_matCachedWVP;

	GMatrix2D        m_mat;

	const GMatrix3D* m_pMatWorld3D;
	GMatrix3D        m_matView3D;
	GMatrix3D        m_matProj3D;

	bool             m_matCached2DDirty;
	bool             m_matCached3DDirty;
	bool             m_matCachedWVPDirty;

	// current color transform for all draw modes
	Cxform m_cxform;

	// system vertex buffer for streaming glyphs
	struct SGlyphVertex
	{
		float        x, y, u, v;
		unsigned int col;
	};
	std::vector<SGlyphVertex, GMemorySTLAlloc<SGlyphVertex>> m_glyphVB;

	// blend mode support
	std::vector<BlendType, GMemorySTLAlloc<BlendType>> m_blendOpStack;
	BlendType m_curBlendMode;

	// render stats
	threadID m_rsIdx;
	Stats    m_renderStats[2];

	// draw parameters
	SSF_GlobalDrawParams* m_pDrawParams;

	SRect                 m_canvasRect;

	float                 m_compDepth;

	// stereo support
	float m_stereo3DiBaseDepth;
	float m_maxParallax;
	float m_customMaxParallax;

	float m_maxParallaxScene;
	float m_screenDepthScene;

	bool  m_stereoFixedProjDepth;
	bool  m_isStereo;
	bool  m_isLeft;

	// lockless rendering
	threadID                m_mainThreadID;
	threadID                m_renderThreadID;
	GRendererCommandBuffer* m_pCmdBuf;

	//Render target management

	struct RenderTargetStackElem
	{
		GTextureXRender* pRT;
		Matrix44         oldViewMat;
		int              oldViewportWidth;
		int              oldViewportHeight;
	};

	std::vector<RenderTargetStackElem, GMemorySTLAlloc<RenderTargetStackElem>> m_renderTargetStack;
	std::vector<GTextureXRender*, GMemorySTLAlloc<GTextureXRender*>>           m_renderTargets;
};

enum EGRendererCommandBufferCmds
{
	GRCBA_Nop,

	//////////////////////////////////////////////////////////////////////////

	//GRCBA_GetRenderCaps,
	//GRCBA_CreateTexture,
	//GRCBA_CreateTextureYUV,

	//GRCBA_CreateRenderTarget,
	//GRCBA_SetDisplayRenderTarget,
	//GRCBA_PushRenderTarget,
	GRCBA_PopRenderTarget,
	GRCBA_PushTempRenderTarget,

	GRCBA_BeginDisplay,
	GRCBA_EndDisplay,

	GRCBA_SetMatrix,
	GRCBA_SetUserMatrix,
	GRCBA_SetCxform,

	GRCBA_PushBlendMode,
	GRCBA_PopBlendMode,

	GRCBA_SetPerspective3D,
	GRCBA_SetView3D,
	GRCBA_SetWorld3D,
	GRCBA_SetWorld3D_NullPtr,

	GRCBA_SetVertexData,
	GRCBA_SetIndexData,
	//GRCBA_ReleaseCachedData,

	GRCBA_DrawIndexedTriList,
	GRCBA_DrawLineStrip,

	GRCBA_LineStyleDisable,
	GRCBA_LineStyleColor,

	GRCBA_FillStyleDisable,
	GRCBA_FillStyleColor,
	GRCBA_FillStyleBitmap,
	GRCBA_FillStyleGouraud,

	GRCBA_DrawBitmaps,

	GRCBA_BeginSubmitMask,
	GRCBA_EndSubmitMask,
	GRCBA_DisableMask,

	//GRCBA_CheckFilterSupport,
	GRCBA_DrawBlurRect,
	GRCBA_DrawColorMatrixRect,

	//GRCBA_GetRenderStats,
	//GRCBA_GetStats,
	//GRCBA_ReleaseResources
};

struct GRendererCommandBuffer
{
	friend class GRendererXRender;

public:
	GRendererCommandBuffer(GMemoryHeap* pHeap)
		: m_curReadPos(0)
		, m_curWritePos(0)
		, m_rawData(GMemorySTLAlloc<unsigned char>(pHeap))
	{
	}

	~GRendererCommandBuffer()
	{
		DropResourceRefs();
	}

	void Reset(size_t initCmdBufSize)
	{
		m_curReadPos = 0;
		m_curWritePos = 0;
		const size_t initCap = initCmdBufSize > 0 ? initCmdBufSize : 2048;
		if (m_rawData.capacity() < initCap)
			m_rawData.reserve(initCap);
	}

	size_t Size() const
	{
		return m_curWritePos;
	}

	size_t Capacity() const
	{
		return m_rawData.capacity();
	}

	void Render(GRendererXRender* pRenderer, const bool releaseResources);
	void DropResourceRefs();

private:
	template<class T> void Playback(T* pRenderer, bool releaseResources);

	void                   ResetReadPos()
	{
		m_curReadPos = 0;
	}

	size_t GetReadPos() const
	{
		assert(IsAligned(m_curReadPos));
		return m_curReadPos;
	}

	const void* GetReadPosPtr() const
	{
		assert(m_curReadPos < Size());
		assert(IsAligned(m_curReadPos));
		return Size() > 0 ? &m_rawData[m_curReadPos] : 0;
	}

	template<typename T>
	void Write(const T& arg)
	{
		assert(IsAligned(m_curWritePos));

		const size_t cellSize = Align(sizeof(T));
		Grow(cellSize);

		const size_t writePos = m_curWritePos;
		*((T*) &m_rawData[writePos]) = arg;
		m_curWritePos += cellSize;
	}

	void Write(const void* pData, const size_t size)
	{
		assert(IsAligned(m_curWritePos));

		if (pData && size)
		{
			Write(size);

			const size_t cellSize = Align(size);
			Grow(cellSize);
			memcpy(&m_rawData[m_curWritePos], pData, size);
			m_curWritePos += cellSize;
		}
		else
		{
			Write((size_t) 0);
		}
	}

	#if defined(_DEBUG)
	void Patch(size_t writePos, size_t data)
	{
		assert(IsAligned(writePos));
		assert(writePos + sizeof(size_t) <= Size());
		*((size_t*) &m_rawData[writePos]) = data;
	}
	#endif

	template<typename T>
	T Read()
	{
		const size_t readPos = m_curReadPos;
		const size_t cellSize = Align(sizeof(T));

		assert(IsAligned(readPos));
		assert(readPos + cellSize <= Size());

		T arg = *((T*) &m_rawData[readPos]);
		m_curReadPos += cellSize;

		return arg;
	}

	template<typename T>
	const T& ReadRef()
	{
		const size_t readPos = m_curReadPos;
		const size_t cellSize = Align(sizeof(T));

		assert(IsAligned(readPos));
		assert(readPos + cellSize <= Size());

		const T& arg = *((const T*) &m_rawData[readPos]);
		m_curReadPos += cellSize;

		return arg;
	}

	void Skip(const size_t size, const bool alignSize)
	{
		assert(IsAligned(m_curReadPos));
		const size_t cellSize = alignSize ? Align(size) : size;
		assert(m_curReadPos + cellSize <= Size());
		m_curReadPos += cellSize;
	}

	void Grow(size_t toWrite)
	{
		size_t bufSize = m_rawData.size();
		size_t writePos = m_curWritePos;
		assert(writePos <= bufSize);
		if (writePos + toWrite > bufSize)
			m_rawData.resize(bufSize + toWrite);
	}

	size_t Align(size_t pos) const
	{
		return (pos + 3) & ~3;
	}

	bool IsAligned(size_t pos) const
	{
		return (pos & 3) == 0;
	}

private:
	size_t m_curReadPos;
	size_t m_curWritePos;
	std::vector<unsigned char, GMemorySTLAlloc<unsigned char>> m_rawData;
};

#endif // #ifdef INCLUDE_SCALEFORM_SDK

#endif // #ifndef _GRENDERER_XRENDER_H_
