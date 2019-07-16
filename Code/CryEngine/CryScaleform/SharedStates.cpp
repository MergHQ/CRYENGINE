// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#ifdef INCLUDE_SCALEFORM_SDK

	#include <CryMath/Cry_Math.h>
	#include <CrySystem/Scaleform/ConfigScaleform.h>
	#include "SharedStates.h"

	#include "FlashPlayerInstance.h"
	#include "GFxVideoWrapper.h"
	#include "GFileCryPak.h"
	#include "Renderer/SFTexture.h"

	#include <CrySystem/IConsole.h>
	#include <CryString/StringUtils.h>
	#include <CryString/CryPath.h>
	#include <CrySystem/ILocalizationManager.h>
	#include <CryRenderer/ITexture.h>
	#include <CrySystem/IStreamEngine.h>
	#include <CryString/UnicodeFunctions.h>
	#include "GFx/GFx_ImageCreator.h"

	#ifndef GFC_NO_THREADSUPPORT

namespace Cry {
namespace Scaleform4 {

using namespace Scaleform::GFx;

//////////////////////////////////////////////////////////////////////////
// Convert custom CryEngine thread priority to a GFx acceptable thread priority value

GThread::ThreadPriority ConvertToGFxThreadPriority(int32 nPriority)
{
	if (nPriority > THREAD_PRIORITY_HIGHEST)
		return GThread::CriticalPriority;
	else if (nPriority > THREAD_PRIORITY_ABOVE_NORMAL)
		return GThread::HighestPriority;
	else if (nPriority > THREAD_PRIORITY_NORMAL)
		return GThread::AboveNormalPriority;
	else if (nPriority > THREAD_PRIORITY_BELOW_NORMAL)
		return GThread::NormalPriority;
	else if (nPriority > THREAD_PRIORITY_LOWEST)
		return GThread::BelowNormalPriority;
	else
		return GThread::LowestPriority;
}
	#endif

//////////////////////////////////////////////////////////////////////////
// CryGFxFileOpener

CryGFxFileOpener::CryGFxFileOpener()
{
}

CryGFxFileOpener& CryGFxFileOpener::GetAccess()
{
	static auto s_pInst = new CryGFxFileOpener(); // This pointer is released in ~GFxLoader2()
	return *s_pInst;
}

CryGFxFileOpener::~CryGFxFileOpener()
{
}

GFile* CryGFxFileOpener::OpenFile(const char* pUrl, int flags, int /*mode*/)
{
	assert(pUrl);
	if (flags & ~(GFileConstants::Open_Read | GFileConstants::Open_Buffered))
		return 0;

	#if defined(USE_GFX_VIDEO)
	const char* pExt = PathUtil::GetExt(pUrl);
	if (!strcmp(pUrl, internal_video_player))
		return new GMemoryFile(pUrl, fxvideoplayer_swf, sizeof(fxvideoplayer_swf));
	if (!stricmp(pExt, "usm"))
		return new GFileCryStream(pUrl, eStreamTaskTypeVideo);
	#endif
	// delegate all file I/O to ICryPak by returning a GFileCryPak object
	return new GFileCryPak(pUrl);
}

SInt64 CryGFxFileOpener::GetFileModifyTime(const char* pUrl)
{
	SInt64 fileModTime = 0;
	if (CFlashPlayer::CheckFileModTimeEnabled())
	{
		ICryPak* pPak = gEnv->pCryPak;
		if (pPak)
		{
			FILE* f = pPak->FOpen(pUrl, "rb");
			if (f)
			{
				fileModTime = (SInt64) pPak->GetModificationTime(f);
				pPak->FClose(f);
			}
		}
	}
	return fileModTime;
}

//////////////////////////////////////////////////////////////////////////
// CryGFxURLBuilder

CryGFxURLBuilder::CryGFxURLBuilder()
{
}

CryGFxURLBuilder& CryGFxURLBuilder::GetAccess()
{
	static auto s_pInst = new CryGFxURLBuilder(); // This pointer is released in ~GFxLoader2()
	return *s_pInst;
}

CryGFxURLBuilder::~CryGFxURLBuilder()
{
}

void CryGFxURLBuilder::BuildURL(GString* pPath, const LocationInfo& loc)
{
	assert(pPath); // GFx always passes a valid dst ptr

	const char* pOrigFilePath = loc.FileName.ToCStr();
	if (pOrigFilePath && *pOrigFilePath)
	{
		// if pOrigFilePath ends with "_locfont.[swf|gfx]", we search for the font in the localization folder
		const char locFontPostFixSwf[] = "_locfont.swf";
		const char locFontPostFixGfx[] = "_locfont.gfx";
		assert(sizeof(locFontPostFixSwf) == sizeof(locFontPostFixGfx)); // both postfixes must be of same length
		const size_t locFontPostFixLen = sizeof(locFontPostFixSwf) - 1;

		size_t filePathLength = strlen(pOrigFilePath);
		if (filePathLength > locFontPostFixLen)
		{
			size_t offset = filePathLength - locFontPostFixLen;
			if (!strnicmp(locFontPostFixGfx, pOrigFilePath + offset, locFontPostFixLen) || !strnicmp(locFontPostFixSwf, pOrigFilePath + offset, locFontPostFixLen))
			{
				char const* const szLanguage = gEnv->pSystem->GetLocalizationManager()->GetLanguage();
				*pPath = PathUtil::GetLocalizationFolder() + CRY_NATIVE_PATH_SEPSTR + szLanguage + "_xml" + CRY_NATIVE_PATH_SEPSTR;
				*pPath += PathUtil::GetFile(pOrigFilePath);
				return;
			}
		}

		if (*pOrigFilePath != '/')
			*pPath = loc.ParentPath + loc.FileName;
		else
			*pPath = GString(&pOrigFilePath[1], filePathLength - 1);
	}
	else
	{
		assert(0 && "CryGFxURLBuilder::BuildURL() - no filename passed!");
		*pPath = "";
	}
}

//////////////////////////////////////////////////////////////////////////
// CryGFxTextClipboard

CryGFxTextClipboard::CryGFxTextClipboard()
	#if CRY_PLATFORM_WINDOWS
	: m_bSyncingClipboardFromWindows(false)
	#endif // CRY_PLATFORM_WINDOWS
{
	if (auto pSystem = gEnv->pSystem)
	{
	#if CRY_PLATFORM_WINDOWS
		const HWND hWnd = reinterpret_cast<HWND>(pSystem->GetHWND());
		AddClipboardFormatListener(hWnd);
		HandleMessage(hWnd, WM_CLIPBOARDUPDATE, 0, 0, nullptr); // Sync current clipboard content with Scaleform
	#endif                                                    // CRY_PLATFORM_WINDOWS
		pSystem->RegisterWindowMessageHandler(this);
	}
}

CryGFxTextClipboard::~CryGFxTextClipboard()
{
	if (gEnv && gEnv->pSystem)
	{
		gEnv->pSystem->UnregisterWindowMessageHandler(this);
	#if CRY_PLATFORM_WINDOWS
		RemoveClipboardFormatListener(reinterpret_cast<HWND>(gEnv->pSystem->GetHWND()));
	#endif // CRY_PLATFORM_WINDOWS
	}
}

CryGFxTextClipboard& CryGFxTextClipboard::GetAccess()
{
	static auto s_pInst = new CryGFxTextClipboard(); // This pointer is released in ~GFxLoader2()
	return *s_pInst;
}

void CryGFxTextClipboard::OnTextStore(const wchar_t* szText, UPInt length)
{
	#if CRY_PLATFORM_WINDOWS
	// Copy to windows clipboard
	if (!m_bSyncingClipboardFromWindows && OpenClipboard(nullptr) != 0)
	{
		const HWND hWnd = reinterpret_cast<HWND>(gEnv->pSystem->GetHWND());

		// Avoid endless notification loop
		RemoveClipboardFormatListener(hWnd);

		static_assert(sizeof(wchar_t) == 2, "sizeof(wchar_t) needs to be 2 to be compatible with Scaleform.");
		const HGLOBAL clipboardData = GlobalAlloc(GMEM_DDESHARE, sizeof(wchar_t) * (length + 1));

		wchar_t* const pData = reinterpret_cast<wchar_t*>(GlobalLock(clipboardData));
		SFwcscpy(pData, length + 1, szText);
		GlobalUnlock(clipboardData);

		SetClipboardData(CF_UNICODETEXT, clipboardData);

		CloseClipboard();

		AddClipboardFormatListener(hWnd);
	}
	#endif // CRY_PLATFORM_WINDOWS
}

	#if CRY_PLATFORM_WINDOWS
bool CryGFxTextClipboard::HandleMessage(CRY_HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (uMsg == WM_CLIPBOARDUPDATE)
	{
		if (OpenClipboard(nullptr) != 0)
		{
			wstring data;
			const HANDLE wideData = GetClipboardData(CF_UNICODETEXT);
			if (wideData)
			{
				const LPCWSTR pWideData = reinterpret_cast<LPCWSTR>(GlobalLock(wideData));
				if (pWideData)
				{
					// Note: This conversion is just to make sure we discard malicious or malformed data
					Unicode::ConvertSafe<Unicode::eErrorRecovery_Discard>(data, pWideData);
					GlobalUnlock(wideData);
				}
			}
			CloseClipboard();

			m_bSyncingClipboardFromWindows = true;
			SetText(data.c_str(), data.size());
			m_bSyncingClipboardFromWindows = false;
		}
	}

	return false;
}
	#endif // CRY_PLATFORM_WINDOWS

//////////////////////////////////////////////////////////////////////////
// CryGFxTranslator

CryGFxTranslator::CryGFxTranslator(unsigned wwMode)
	: GFxTranslator(wwMode), m_pILocMan(0)
{
}

CryGFxTranslator& CryGFxTranslator::GetAccess(bool createNew)
{
	static GPtr<CryGFxTranslator> s_pTranslatorInst(*new CryGFxTranslator(WWT_Default));
	if (createNew)
	{
		s_pTranslatorInst = *new CryGFxTranslator(s_pTranslatorInst->WWMode);
	}
	return *s_pTranslatorInst;
}

void CryGFxTranslator::OnLanguageChanged()
{
	CryGFxTranslator& translator = GetAccess(true);
	GFxLoader* pLoader = CSharedFlashPlayerResources::GetAccess().GetLoader(true);
	if (pLoader)
	{
		pLoader->SetTranslator(&translator);
	}
}

void CryGFxTranslator::SetWordWrappingMode(const char* pLanguage)
{
	if (!stricmp(pLanguage, "japanese"))
	{
		WWMode = WWT_Prohibition;
	}
	else if (!stricmp(pLanguage, "chineset"))
	{
		WWMode = WWT_Asian;
	}
	else
	{
		WWMode = WWT_Default;
	}
}

CryGFxTranslator::~CryGFxTranslator()
{
}

UInt CryGFxTranslator::GetCaps() const
{
	return Cap_ReceiveHtml | Cap_StripTrailingNewLines;
}

void CryGFxTranslator::Translate(TranslateInfo* pTranslateInfo)
{
	IF (m_pILocMan == 0, 0)
		m_pILocMan = gEnv->pSystem->GetLocalizationManager();

	if (!m_pILocMan || !pTranslateInfo)
		return;

	const wchar_t* pKey = pTranslateInfo->GetKey();

	if (!pKey) // No string -> not localizable
		return;

	string utf8Key, localizedString;
	Unicode::Convert(utf8Key, pKey);

	if (m_pILocMan->LocalizeString(utf8Key, localizedString))
	{
		if (pTranslateInfo->IsKeyHtml())
			pTranslateInfo->SetResultHtml(localizedString.c_str());
		else
			pTranslateInfo->SetResult(localizedString.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////
// CryGFxLog

ICVar* CryGFxLog::CV_sys_flash_debuglog(0);
int CryGFxLog::ms_sys_flash_debuglog(0);

	#if defined(CRYGFXLOG_USE_IMPLICT_TLS)
		#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO
static __declspec(thread) const char* g_pCurFlashContext = 0;
		#elif CRY_PLATFORM_ORBIS
static __thread const char* g_pCurFlashContext = 0;
		#elif CRY_PLATFORM_LINUX || CRY_PLATFORM_ANDROID || CRY_PLATFORM_APPLE
static __thread const char* g_pCurFlashContext = 0;
		#else
			#error No TLS storage defined for CryGFxLog on this platform config
		#endif
	#else
static unsigned int g_curFlashContextTlsIdx = TLS_OUT_OF_INDEXES;
	#endif

CryGFxLog::CryGFxLog()
	: m_pLog(0)
{
	m_pLog = gEnv->pLog;

	if (!CV_sys_flash_debuglog)
		CV_sys_flash_debuglog = REGISTER_CVAR2("sys_flash_debuglog", &ms_sys_flash_debuglog, 0, VF_NULL, "");

	#if !defined(CRYGFXLOG_USE_IMPLICT_TLS)
	g_curFlashContextTlsIdx = TlsAlloc();
	assert(g_curFlashContextTlsIdx != TLS_OUT_OF_INDEXES);
	#endif
}

CryGFxLog& CryGFxLog::GetAccess()
{
	static auto s_pInst = new CryGFxLog(); // This pointer is released in ~GFxLoader2()
	return *s_pInst;
}

CryGFxLog::~CryGFxLog()
{
	#if !defined(CRYGFXLOG_USE_IMPLICT_TLS)
	if (g_curFlashContextTlsIdx != TLS_OUT_OF_INDEXES)
		TlsFree(g_curFlashContextTlsIdx);
	#endif
}

void CryGFxLog::LogMessageVarg(Scaleform::LogMessageId messageType, const char* pFmt, va_list argList)
{
	char logBuf[1024] = "<Flash> ";
	{
		// msg
		size_t len = strlen(logBuf);
		{
			cry_vsprintf(&logBuf[len], sizeof(logBuf) - len, pFmt, argList);
			len = strlen(logBuf);
			if (logBuf[len - 1] == '\n')
			{
				logBuf[--len] = 0;
			}
		}

		// context
		{
	#if defined(CRYGFXLOG_USE_IMPLICT_TLS)
			const char* pCurFlashContext = g_pCurFlashContext;
	#else
			const char* pCurFlashContext = g_curFlashContextTlsIdx != TLS_OUT_OF_INDEXES ? (const char*) TlsGetValue(g_curFlashContextTlsIdx) : 0;
	#endif
			cry_sprintf(&logBuf[len], sizeof(logBuf) - len, " [%s]", pCurFlashContext ? pCurFlashContext : "#!NO_CONTEXT!#");
		}
	}

	if (ms_sys_flash_debuglog)
	{
		PREFAST_SUPPRESS_WARNING(6053);
		OutputDebugString(logBuf);
	}

	switch (messageType.GetMessageType())
	{
	case LogMessage_Error:
		{
			m_pLog->LogError("%s", logBuf);
			break;
		}
	case LogMessage_Warning:
		{
			m_pLog->LogWarning("%s", logBuf);
			break;
		}
	case LogMessage_Text:
	default:
		{
			m_pLog->Log("%s", logBuf);
			break;
		}
	}
}

void CryGFxLog::SetContext(const char* pFlashContext)
{
	#if defined(CRYGFXLOG_USE_IMPLICT_TLS)
	g_pCurFlashContext = pFlashContext;
	#else
	if (g_curFlashContextTlsIdx != TLS_OUT_OF_INDEXES)
		TlsSetValue(g_curFlashContextTlsIdx, (void*) pFlashContext);
	#endif
}

const char* CryGFxLog::GetContext() const
{
	#if defined(CRYGFXLOG_USE_IMPLICT_TLS)
	return g_pCurFlashContext;
	#else
	const char* pCurFlashContext = g_curFlashContextTlsIdx != TLS_OUT_OF_INDEXES ? (const char*) TlsGetValue(g_curFlashContextTlsIdx) : 0;
	return pCurFlashContext;
	#endif
}

//////////////////////////////////////////////////////////////////////////
// CryGFxFSCommandHandler

CryGFxFSCommandHandler::CryGFxFSCommandHandler()
{
}

CryGFxFSCommandHandler& CryGFxFSCommandHandler::GetAccess()
{
	static auto s_pInst = new CryGFxFSCommandHandler(); // This pointer is released in ~GFxLoader2()
	return *s_pInst;
}

CryGFxFSCommandHandler::~CryGFxFSCommandHandler()
{
}

void CryGFxFSCommandHandler::Callback(GFxMovieView* pMovieView, const char* pCommand, const char* pArgs)
{
	// get flash player instance this action script command belongs to and delegate it to client
	CFlashPlayer* pFlashPlayer(static_cast<CFlashPlayer*>(pMovieView->GetUserData()));
	if (pFlashPlayer)
		pFlashPlayer->DelegateFSCommandCallback(pCommand, pArgs);
}

//////////////////////////////////////////////////////////////////////////
// CryGFxExternalInterface

CryGFxExternalInterface::CryGFxExternalInterface()
{
}

CryGFxExternalInterface& CryGFxExternalInterface::GetAccess()
{
	static auto s_pInst = new CryGFxExternalInterface(); // This pointer is released in ~GFxLoader2()
	return *s_pInst;
}

CryGFxExternalInterface::~CryGFxExternalInterface()
{
}

void CryGFxExternalInterface::Callback(GFxMovieView* pMovieView, const char* pMethodName, const GFxValue* pArgs, UInt numArgs)
{
	CFlashPlayer* pFlashPlayer(static_cast<CFlashPlayer*>(pMovieView->GetUserData()));
	if (pFlashPlayer)
		pFlashPlayer->DelegateExternalInterfaceCallback(pMethodName, pArgs, numArgs);
	else
		pMovieView->SetExternalInterfaceRetVal(GFxValue(GFxValue::VT_Undefined));
}

//////////////////////////////////////////////////////////////////////////
// CryGFxUserEventHandler

CryGFxUserEventHandler::CryGFxUserEventHandler()
{
}

CryGFxUserEventHandler& CryGFxUserEventHandler::GetAccess()
{
	static auto s_pInst = new CryGFxUserEventHandler(); // This pointer is released in ~GFxLoader2()
	return *s_pInst;
}

CryGFxUserEventHandler::~CryGFxUserEventHandler()
{
}

void CryGFxUserEventHandler::HandleEvent(GFxMovieView* /*pMovieView*/, const GFxEvent& /*event*/)
{
	// not implemented since it's not needed yet... would need to translate GFx event to independent representation
}

//////////////////////////////////////////////////////////////////////////
// CryImageCreator

Image* CryImageCreator::LoadProtocolImage(const Scaleform::GFx::ImageCreateInfo& info, const Scaleform::String& url)
{
	const char* pFilePath(strstr(url, "://"));
	if (!pFilePath)
	{
		return ImageCreator::LoadProtocolImage(info, url);
	}

	if (pFilePath)
	{
		pFilePath += 3;
	}
	return LoadImageFile(info, pFilePath);
}

Image* CryImageCreator::LoadImageFile(const ImageCreateInfo& info, const Scaleform::String& url)
{
	IScaleformRecording* pRenderer = CSharedFlashPlayerResources::GetAccess().GetRenderer(true);
	CRY_ASSERT(pRenderer && pRenderer->GetTextureManager());
	if (!pRenderer || !pRenderer->GetTextureManager())
	{
		return ImageCreator::LoadImageFile(info, url);
	}

	ImageSize dimensions(0, 0);
	UByte mipLevels = 0;
	const Scaleform::Render::SSFFormat* pFormat = nullptr;
	Scaleform::GFx::ImageFileHandlerRegistry* registry = info.GetImageFileHandlerRegistry();
	Scaleform::Render::CSFTextureManager* pTextureManager = static_cast<Scaleform::Render::CSFTextureManager*>(pRenderer->GetTextureManager());

	if (registry && info.GetFileOpener())
	{
		GPtr<Scaleform::File> file = *info.GetFileOpener()->OpenFile(url);
		if (!file || !file->IsValid())
		{
			return nullptr;
		}

		ImageFileReader* reader;
		if (registry->DetectFormat(&reader, file) != Scaleform::Render::ImageFile_Unknown)
		{
			ImageCreateArgs args;
			args.pHeap = info.pHeap;
			args.Use = info.Use;
			args.pManager = GetTextureManager();
			CRY_ASSERT(info.RUse != Resource::Use_FontTexture);

			if (info.RUse == Resource::Use_FontTexture)
			{
				args.Format = Scaleform::Render::Image_A8;
			}

			GPtr<ImageSource> source = *reader->ReadImageSource(file, args);
			if (source)
			{
				dimensions = source->GetSize();
				mipLevels = source->GetMipmapCount();
				pFormat = pTextureManager->FindFormat(source->GetFormat());
			}
		}
	}

	CRY_ASSERT(0 != dimensions.Width && 0 != dimensions.Height && 0 != mipLevels && pFormat);
	GPtr<Scaleform::Render::Texture> pSFtexture = *pTextureManager->CreateTexture(url, dimensions, mipLevels, pFormat);
	CRY_ASSERT(pSFtexture);

	if (!pSFtexture)
	{
		return ImageCreator::LoadImageFile(info, url);
	}

	return SF_HEAP_AUTO_NEW(pRenderer) CryTextureImage(pSFtexture, info.Use);
}

} // ~Scaleform4 namespace
} // ~Cry namespace

#endif // #ifdef INCLUDE_SCALEFORM_SDK
