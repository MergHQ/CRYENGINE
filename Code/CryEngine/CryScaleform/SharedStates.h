// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef _SHARED_STATES_H_
#define _SHARED_STATES_H_

#pragma once

#ifdef INCLUDE_SCALEFORM_SDK

	#include "ScaleformTypes.h"

	#pragma warning(push)
	#pragma warning(disable : 6326)// Potential comparison of a constant with another constant
	#pragma warning(disable : 6011)// Dereferencing NULL pointer
	#include <CryCore/Platform/CryWindows.h>
	#include <CryCore/BaseTypes.h>
	#include <CrySystem/IWindowMessageHandler.h>
	#pragma warning(pop)

struct ILocalizationManager;
struct ILog;
struct ICVar;

namespace Cry {
namespace Scaleform4 {

	#ifndef GFC_NO_THREADSUPPORT
//////////////////////////////////////////////////////////////////////////
// Convert custom CryEngine thread priority to a GFx acceptable thread priority value

GThread::ThreadPriority ConvertToGFxThreadPriority(int32 nCryPriority);
	#endif

//////////////////////////////////////////////////////////////////////////
// CryGFxFileOpener

class CryGFxFileOpener : public GFxFileOpener
{
public:
	// GFxFileOpener interface
	virtual GFile* OpenFile(const char* pUrl, SInt flags = GFileConstants::Open_Read | GFileConstants::Open_Buffered, SInt mode = GFileConstants::Mode_ReadWrite);
	virtual SInt64 GetFileModifyTime(const char* pUrl);
	// no overwrite for OpenFileEx needed yet as it calls out OpenFile impl
	//virtual GFile* OpenFileEx(const char* pUrl, class GFxLog *pLog, SInt flags = GFileConstants::Open_Read|GFileConstants::Open_Buffered, SInt mode = GFileConstants::Mode_ReadWrite);

public:
	static CryGFxFileOpener& GetAccess();
	~CryGFxFileOpener();

private:
	CryGFxFileOpener();
};

//////////////////////////////////////////////////////////////////////////
// CryGFxURLBuilder

class CryGFxURLBuilder : public GFxURLBuilder
{
	// GFxURLBuilder interface
	virtual void BuildURL(GString* pPath, const LocationInfo& loc);

public:
	static CryGFxURLBuilder& GetAccess();
	~CryGFxURLBuilder();

private:
	CryGFxURLBuilder();
};

//////////////////////////////////////////////////////////////////////////
// CryGFxTextClipboard

class CryGFxTextClipboard : public GFxTextClipboard, IWindowMessageHandler
{
public:
	void                        OnTextStore(const wchar_t* szText, UPInt length) override;
	static CryGFxTextClipboard& GetAccess();
	~CryGFxTextClipboard();

	#if CRY_PLATFORM_WINDOWS
	bool HandleMessage(CRY_HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
	#endif // CRY_PLATFORM_WINDOWS

private:
	CryGFxTextClipboard();

	#if CRY_PLATFORM_WINDOWS
	bool m_bSyncingClipboardFromWindows;
	#endif // CRY_PLATFORM_WINDOWS
};

//////////////////////////////////////////////////////////////////////////
// CryGFxTranslator

class CryGFxTranslator : public GFxTranslator
{
public:
	// GFxTranslator interface
	virtual unsigned GetCaps() const;
	virtual void     Translate(TranslateInfo* pTranslateInfo);
	static void      OnLanguageChanged();

public:
	static CryGFxTranslator& GetAccess(bool createNew = false);
	~CryGFxTranslator();

	void SetWordWrappingMode(const char* pLanguage);

private:
	CryGFxTranslator(unsigned wwMode);

private:
	ILocalizationManager* m_pILocMan;
	bool                  m_bDirty;
};

//////////////////////////////////////////////////////////////////////////
// CryGFxLog

	#if CRY_PLATFORM_ORBIS || CRY_PLATFORM_APPLE || CRY_PLATFORM_LINUX || CRY_PLATFORM_ANDROID
		#define CRYGFXLOG_USE_IMPLICT_TLS
	#endif

class CryGFxLog : public GFxLog
{
public:
	// GFxLog interface
	virtual void LogMessageVarg(Scaleform::LogMessageId messageType, const char* pFmt, va_list argList);

public:
	static CryGFxLog& GetAccess();
	~CryGFxLog();

	void        SetContext(const char* pFlashContext);
	const char* GetContext() const;

private:
	CryGFxLog();

private:
	static ICVar* CV_sys_flash_debuglog;
	static int    ms_sys_flash_debuglog;

private:
	ILog* m_pLog;
};

//////////////////////////////////////////////////////////////////////////
// CryGFxFSCommandHandler

class CryGFxFSCommandHandler : public GFxFSCommandHandler
{
public:
	// GFxFSCommandHandler interface
	virtual void Callback(GFxMovieView* pMovieView, const char* pCommand, const char* pArgs);

public:
	static CryGFxFSCommandHandler& GetAccess();
	~CryGFxFSCommandHandler();

private:
	CryGFxFSCommandHandler();
};

//////////////////////////////////////////////////////////////////////////
// CryGFxExternalInterface

class CryGFxExternalInterface : public GFxExternalInterface
{
public:
	// GFxExternalInterface interface
	virtual void Callback(GFxMovieView* pMovieView, const char* pMethodName, const GFxValue* pArgs, UInt numArgs);

public:
	static CryGFxExternalInterface& GetAccess();
	~CryGFxExternalInterface();

private:
	CryGFxExternalInterface();
};

//////////////////////////////////////////////////////////////////////////
// CryGFxUserEventHandler

class CryGFxUserEventHandler : public GFxUserEventHandler
{
public:
	// GFxUserEventHandler interface
	virtual void HandleEvent(GFxMovieView* pMovieView, const GFxEvent& event);

public:
	static CryGFxUserEventHandler& GetAccess();
	~CryGFxUserEventHandler();

private:
	CryGFxUserEventHandler();
};

//////////////////////////////////////////////////////////////////////////
// CryTextureImage

class CryTextureImage : public Scaleform::Render::TextureImage
{
public:
	CryTextureImage(Scaleform::Render::Texture* pTexture, unsigned int use)
		: Scaleform::Render::TextureImage(
			Scaleform::Render::Image_None
			, Scaleform::Render::ImageSize(0, 0)
			, use
			, pTexture)
	{
		RefreshCachedFormat();
	}
};

//////////////////////////////////////////////////////////////////////////
// CryImageCreator

class CryImageCreator : public Scaleform::GFx::ImageCreator
{
public:
	CryImageCreator(Scaleform::Render::TextureManager* pTextureManager) : Scaleform::GFx::ImageCreator(pTextureManager) {}

	// Looks up image for "img://" protocol.
	virtual Image* LoadProtocolImage(const Scaleform::GFx::ImageCreateInfo& info, const Scaleform::String& url) override;
	// Loads image from file.
	virtual Image* LoadImageFile(const Scaleform::GFx::ImageCreateInfo& info, const Scaleform::String& url) override;
};

} // ~Scaleform4 namespace
} // ~Cry namespace

#endif // #ifdef INCLUDE_SCALEFORM_SDK

#endif // #ifndef _SHARED_STATES_H_
