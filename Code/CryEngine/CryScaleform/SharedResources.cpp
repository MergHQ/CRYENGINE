// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#ifdef INCLUDE_SCALEFORM_SDK

	#include "SharedResources.h"
	#include "SharedStates.h"
	#include "GFxVideoWrapper.h"
	#include "FlashPlayerInstance.h"
	#include "GImeHelper.h"
	#include "Renderer/SFRenderer.h"
	#include <CryMemory/CrySizer.h>

	#include <CryRenderer/IScaleform.h>

namespace Cry {
namespace Scaleform4 {

	#ifdef GHEAP_TRACE_ALL

//////////////////////////////////////////////////////////////////////////
// CryMemReplayGHeapTracer

class CryMemReplayGHeapTracer : public GMemoryHeap::HeapTracer
{
public:
	void OnCreateHeap(const GMemoryHeap* heap)
	{
	}

	void OnDestroyHeap(const GMemoryHeap* heap)
	{
	}

	void OnAlloc(const GMemoryHeap* heap, UPInt size, UPInt align, unsigned sid, const void* ptr)
	{
		MEMREPLAY_SCOPE(EMemReplayAllocClass::UserPointer, EMemReplayUserPointerClass::CryMalloc);
		MEMREPLAY_SCOPE_ALLOC(ptr, size, align);
	}

	void OnRealloc(const GMemoryHeap* heap, const void* oldPtr, UPInt newSize, const void* newPtr)
	{
		MEMREPLAY_SCOPE(EMemReplayAllocClass::UserPointer, EMemReplayUserPointerClass::CryMalloc);
		MEMREPLAY_SCOPE_REALLOC(oldPtr, newPtr, newSize, 0);
	}

	void OnFree(const GMemoryHeap* heap, const void* ptr)
	{
		MEMREPLAY_SCOPE(EMemReplayAllocClass::UserPointer, EMemReplayUserPointerClass::CryMalloc);
		MEMREPLAY_SCOPE_FREE(ptr);
	}
};

	#endif

//////////////////////////////////////////////////////////////////////////
// GSystemInitWrapper

class GSystemInitWrapper
{
public:
	GSystemInitWrapper()
		: m_pGFxMemInterface(0)
	{
		size_t staticPoolSize = CFlashPlayer::GetStaticPoolSize();
		if (staticPoolSize)
		{
			gEnv->pLog->Log("  Using static pool of %.1f MB", staticPoolSize / (1024.0f * 1024.0f));
			GSysAllocStaticCryMem* pStaticAlloc = new GSysAllocStaticCryMem(staticPoolSize);

			if (!pStaticAlloc || !pStaticAlloc->IsValid())
			{
				gEnv->pLog->LogWarning("  Failed to allocate memory for static pool! Switching to dynamic pool.");
				SAFE_DELETE(pStaticAlloc);
			}
			else
				m_pGFxMemInterface = pStaticAlloc;
		}

		if (!m_pGFxMemInterface)
		{
			gEnv->pLog->Log("  Using dynamic pool");
			m_pGFxMemInterface = new GSysAllocCryMem(CFlashPlayer::GetAddressSpaceSize());
		}

		assert(m_pGFxMemInterface && m_pGFxMemInterface->GetSysAllocImpl());
		GSystem::Init(m_pGFxMemInterface->GetSysAllocImpl());

	#ifdef GHEAP_TRACE_ALL
		GMemory::GetGlobalHeap()->SetTracer(&m_heapTracer);
	#endif
	}

	~GSystemInitWrapper()
	{
	#ifdef GHEAP_TRACE_ALL
		GMemory::GetGlobalHeap()->SetTracer(NULL);
	#endif

	#if !defined(_RELEASE)
		if (AreCustomMemoryArenasActive())
		{
			CryGFxLog::GetAccess().LogError("Still using custom memory arenas while shutting down GFx memory system! Enforce breaking into the debugger...");
			__debugbreak();
		}
	#endif

		GSystem::Destroy();

		SAFE_DELETE(m_pGFxMemInterface);
	}

	CryGFxMemInterface* GetGFxMemInterface() { return m_pGFxMemInterface; }

	void                GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_pGFxMemInterface);
	}

	int CreateMemoryArena(unsigned int arenaID, bool resetCache) const
	{
		assert(m_pGFxMemInterface);
		return m_pGFxMemInterface->GetMemoryArenas().Create(arenaID, resetCache);
	}

	void DestoryMemoryArena(unsigned int arenaID) const
	{
		assert(m_pGFxMemInterface);
		m_pGFxMemInterface->GetMemoryArenas().Destroy(arenaID);
	}

	bool AreCustomMemoryArenasActive() const
	{
		assert(m_pGFxMemInterface);
		return m_pGFxMemInterface->GetMemoryArenas().AnyActive();
	}

	float GetFlashHeapFragmentation() const
	{
		assert(m_pGFxMemInterface);
		return m_pGFxMemInterface->GetFlashHeapFragmentation();
	}

private:
	CryGFxMemInterface* m_pGFxMemInterface;

	#ifdef GHEAP_TRACE_ALL
	CryMemReplayGHeapTracer m_heapTracer;
	#endif
};

//////////////////////////////////////////////////////////////////////////
// CSharedFlashPlayerResources

CSharedFlashPlayerResources* CSharedFlashPlayerResources::ms_pSharedFlashPlayerResources = 0;

CSharedFlashPlayerResources& CSharedFlashPlayerResources::GetAccess()
{
	assert(ms_pSharedFlashPlayerResources);
	return *ms_pSharedFlashPlayerResources;
}

CSharedFlashPlayerResources::CSharedFlashPlayerResources()
	: m_pGSystemInit(0)
	, m_pLoader(0)
	, m_pRecorder(0)
	, m_pMeshCacheResetThread(0)
	#if defined(USE_GFX_IME)
	, m_pImeHelper(0)
	#endif
{
	CRY_PROFILE_FUNCTION(PROFILE_LOADING_ONLY);

	gEnv->pLog->Log("Using Scaleform GFx " GFC_FX_VERSION_STRING);
	m_pGSystemInit = new GSystemInitWrapper();
	assert(m_pGSystemInit);

	SingleThreadCommandQueue* pCommandQueue = new SingleThreadCommandQueue;
	m_pRecorder = new Scaleform::Render::CSFRenderer(pCommandQueue);
	pCommandQueue->pHAL = m_pRecorder;

	threadID mainThread = 0;
	threadID renderThread = 0;
	gEnv->pRenderer->GetThreadIDs(mainThread, renderThread);
	m_pRecorder->InitHAL(Scaleform::Render::HALInitParams(0, Scaleform::Render::HALConfig_DisableImplicitTarget, (Scaleform::ThreadId)renderThread));

	m_pImageFileRegistry = new Scaleform::GFx::ImageFileHandlerRegistry(Scaleform::GFx::ImageFileHandlerRegistry::AddDefaultHandlers);
	m_pImageCreator = new CryImageCreator(m_pRecorder->GetTextureManager());
	m_pLoader = new GFxLoader2();
	m_pLoader->SetImageFileHandlerRegistry(m_pImageFileRegistry);
	m_pLoader->SetImageCreator(m_pImageCreator);

	#if defined(USE_GFX_IME)
	m_pImeHelper = new GImeHelper();
	if (!m_pImeHelper->ApplyToLoader(m_pLoader))
	{
		SAFE_RELEASE(m_pImeHelper);
	}
	#endif
}

CSharedFlashPlayerResources::~CSharedFlashPlayerResources()
{
	#if defined(USE_GFX_IME)
	// IME must be first to go since it has stuff allocated that needs to be released before we shut down the rest
	SAFE_RELEASE(m_pImeHelper);
	#endif

	CFlashPlayer::DumpAndFixLeaks();
	m_pRecorder->ShutdownHAL();

	assert(!m_pLoader || m_pLoader->GetRefCount() == 1);
	SAFE_RELEASE(m_pLoader);
	assert(!m_pRecorder || m_pRecorder->GetRefCount() == 1);
	SAFE_DELETE(m_pImageCreator);
	SAFE_DELETE(m_pImageFileRegistry);
	SAFE_RELEASE(m_pRecorder);
	SAFE_DELETE(m_pGSystemInit);
}

void CSharedFlashPlayerResources::Init()
{
	CRY_PROFILE_FUNCTION(PROFILE_LOADING_ONLY);
	assert(!ms_pSharedFlashPlayerResources);
	static char s_sharedFlashPlayerResourcesStorage[sizeof(CSharedFlashPlayerResources)] = { 0 };
	if (!ms_pSharedFlashPlayerResources)
		ms_pSharedFlashPlayerResources = new(s_sharedFlashPlayerResourcesStorage) CSharedFlashPlayerResources;
}

void CSharedFlashPlayerResources::Shutdown()
{
	assert(ms_pSharedFlashPlayerResources);
	if (ms_pSharedFlashPlayerResources)
	{
		ms_pSharedFlashPlayerResources->~CSharedFlashPlayerResources();
		ms_pSharedFlashPlayerResources = 0;
	}
}

GFxLoader2* CSharedFlashPlayerResources::GetLoader(bool getRawInterface)
{
	assert(m_pLoader);
	if (!getRawInterface)
		m_pLoader->AddRef();
	return m_pLoader;
}

IScaleformRecording* CSharedFlashPlayerResources::GetRenderer(bool getRawInterface /*= false*/)
{
	assert(m_pRecorder);
	if (!getRawInterface)
		m_pRecorder->AddRef();
	return m_pRecorder;
}

CryGFxMemInterface::Stats CSharedFlashPlayerResources::GetSysAllocStats() const
{
	CryGFxMemInterface::Stats stats = { 0 };
	if (m_pGSystemInit)
	{
		CryGFxMemInterface* p = m_pGSystemInit->GetGFxMemInterface();
		if (p)
			stats = p->GetStats();
	}
	return stats;
}

void CSharedFlashPlayerResources::GetMemoryUsage(ICrySizer* pSizer) const
{
	assert(pSizer);
	pSizer->AddObject(m_pGSystemInit);
	pSizer->AddObject(m_pLoader);
}

int CSharedFlashPlayerResources::CreateMemoryArena(unsigned int arenaID, bool resetCache) const
{
	assert(m_pGSystemInit);
	return m_pGSystemInit->CreateMemoryArena(arenaID, resetCache);
}

void CSharedFlashPlayerResources::DestoryMemoryArena(unsigned int arenaID) const
{
	assert(m_pGSystemInit);
	return m_pGSystemInit->DestoryMemoryArena(arenaID);
}

bool CSharedFlashPlayerResources::AreCustomMemoryArenasActive() const
{
	assert(m_pGSystemInit);
	return m_pGSystemInit->AreCustomMemoryArenasActive();
}

void CSharedFlashPlayerResources::ResetMeshCache() const
{
	static_cast<Scaleform::Render::CSFRenderer*>(m_pRecorder)->GetMeshCache().ClearCache();
}

bool CSharedFlashPlayerResources::IsFlashVideoIOStarving() const
{
	#if defined(USE_GFX_VIDEO)
	if (m_pLoader)
	{
		GPtr<GFxVideoBase> pVideo = m_pLoader->GetVideo();
		return pVideo.GetPtr() ? pVideo->IsIORequired() : false;
	}
	#endif
	return false;
}

float CSharedFlashPlayerResources::GetFlashHeapFragmentation() const
{
	assert(m_pGSystemInit);
	return m_pGSystemInit->GetFlashHeapFragmentation();
}

void CSharedFlashPlayerResources::SetImeFocus(GFxMovieView* pMovie, bool bSet)
{
	#if defined(USE_GFX_IME)
	if (m_pImeHelper)
	{
		m_pImeHelper->SetImeFocus(pMovie, bSet);
	}
	#endif
}

//////////////////////////////////////////////////////////////////////////
// GFxLoader2

GFxLoader2::GFxLoader2()
	: GFxLoader(&CryGFxFileOpener::GetAccess(), GFX_LOADER_NEW_ZLIBSUPPORT)
	, m_refCount(1)
	, m_parserVerbosity()
{
	// set callbacks
	SetLog(&CryGFxLog::GetAccess());
	SetURLBuilder(&CryGFxURLBuilder::GetAccess());
	SetTranslator(&CryGFxTranslator::GetAccess());
	SetClipboard(&CryGFxTextClipboard::GetAccess());

	// enable dynamic font cache
	SetupDynamicFontCache();

	// set parser verbosity
	UpdateParserVerbosity();
	SetParseControl(&m_parserVerbosity);

	GPtr<Scaleform::GFx::ASSupport> pASSupport = new Scaleform::GFx::AS3Support();
	SetAS3Support(pASSupport);
	GPtr<Scaleform::GFx::ASSupport> pAS2Support = new Scaleform::GFx::AS2Support();
	SetAS2Support(pAS2Support);

	// set up video
	#if defined(USE_GFX_VIDEO)
	GFxVideoWrapper::SetVideo(this);
	#endif
}

GFxLoader2::~GFxLoader2()
{
	SetLog(nullptr);
	SetURLBuilder(nullptr);
	SetImageCreator(nullptr);
	SetTranslator(nullptr);
	SetClipboard(nullptr);

	CryGFxLog::GetAccess().Release();
	CryGFxURLBuilder::GetAccess().Release();
	CryGFxTranslator::GetAccess().Release();
	CryGFxTextClipboard::GetAccess().Release();

	SetParseControl(0);
}

void GFxLoader2::AddRef()
{
	CryInterlockedIncrement(&m_refCount);
}

void GFxLoader2::Release()
{
	long refCount = CryInterlockedDecrement(&m_refCount);
	if (refCount <= 0)
		delete this;
}

void GFxLoader2::UpdateParserVerbosity()
{
	m_parserVerbosity.SetParseFlags((CFlashPlayer::GetLogOptions() & CFlashPlayer::LO_LOADING) ?
	                                GFxParseControl::VerboseParseAll : GFxParseControl::VerboseParseNone);
}

void GFxLoader2::SetupDynamicFontCache()
{
	SetFontPackParams(0);
}

	#if defined(USE_GFX_VIDEO) && CRY_COMPILER_MSVC && CRY_COMPILER_VERSION >= 1900
// We need this to link the CRI library inside GfxVideo when using compiler VC14 or newer.
static auto* g_vsprintf_s = static_cast<int (*)(char*, size_t, const char*, va_list)>(&vsprintf_s);
static auto* g_sprintf_s = static_cast<int (*)(char*, size_t, const char*, ...)>(&sprintf_s);
	#endif

} // ~Scaleform4 namespace
} // ~Cry namespace

#endif // #ifdef INCLUDE_SCALEFORM_SDK
