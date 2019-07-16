// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GImeHelper.h"
#if defined(INCLUDE_SCALEFORM_SDK) && defined(USE_GFX_IME)
	#include <CrySystem/IImeManager.h>
	#include "SharedResources.h"
	#include "SharedStates.h"

namespace Cry {
namespace Scaleform4 {

inline IImeManager* GetSystemImeManager(ISystem* pSystem)
{
	CRY_ASSERT(pSystem);
	return pSystem->GetImeManager();
}

	#if CRY_PLATFORM_WINDOWS
inline HWND GetSystemHWND(ISystem* pSystem)
{
	CRY_ASSERT(pSystem);
	return static_cast<HWND>(pSystem->GetHWND());
}
	#endif

// Constructor
GImeHelper::GImeHelper() : GFxImeManagerBase(GetSystemHWND(gEnv->pSystem))
{
	m_pCurrentMovie = NULL;
}

// Destructor
GImeHelper::~GImeHelper()
{
	if (gEnv && gEnv->pSystem)
	{
		IImeManager* pImeManager = GetSystemImeManager(gEnv->pSystem);
		if (pImeManager)
		{
			pImeManager->SetScaleformHandler(NULL);
		}
	}
}

// Apply this IME helper to the Scaleform loader
bool GImeHelper::ApplyToLoader(GFxLoader* pLoader)
{
	ISystem* pSystem = gEnv->pSystem;

	if (!GetSystemImeManager(pSystem)->SetScaleformHandler(this))
	{
		// This IME should not be used
		return false;
	}
	// Note: Renamed to .dat file because .xml files can be transformed to binary XML which can't be read by Scaleform
	string gameFolder = pSystem->GetIPak()->GetGameFolder();
	string imeXmlFile = gameFolder + "/Libs/UI/ime.xml";
	if (pLoader && Init(&CryGFxLog::GetAccess(), &CryGFxFileOpener::GetAccess(), imeXmlFile))
	{
		SetIMEMoviePath("IME.swf"); // Note: Will be looked for in the Libs/UI folder
		pLoader->SetIMEManager(this);

		Ptr<Scaleform::GFx::FontProviderWin32> pFontProvider = *new Scaleform::GFx::FontProviderWin32(::GetDC(0));
		pLoader->SetFontProvider(pFontProvider);

		Ptr<Scaleform::GFx::FontLib> pLib = *new Scaleform::GFx::FontLib;
		pLoader->SetFontLib(pLib);

		Ptr<Scaleform::GFx::MovieDef> pDef = *pLoader->CreateMovie("/Libs/UI/fonts_all.swf");
		if (pDef)
		{
			pLib->AddFontsFrom(pDef, 0);
		}

		Scaleform::GFx::DrawTextManager* pDrawTextManager = new Scaleform::GFx::DrawTextManager(pLoader);
		Ptr<Scaleform::GFx::FontProviderHUD> pFontProviderHUD = new Scaleform::GFx::FontProviderHUD();
		pDrawTextManager->SetFontProvider(pFontProviderHUD);

		Ptr<Scaleform::GFx::FontMap> pFontMap = *new Scaleform::GFx::FontMap;
		pFontMap->MapFont("$IMELanguageBar", "Arial Unicode MS");
		pFontMap->MapFont("$NormalFont", "Arial Unicode MS", Scaleform::GFx::FontMap::MFF_Bold);
		pFontMap->MapFont("$IMESample", "Arial Unicode MS");
		pLoader->SetFontMap(pFontMap);
	}
	return true;
}

// Forward the event to the current Scaleform movie
bool GImeHelper::ForwardEvent(GFxIMEEvent& event)
{
	return m_pCurrentMovie && (m_pCurrentMovie->HandleEvent(event) & GFxMovieView::HE_NoDefaultAction) == 0;
}

bool GImeHelper::OnIMEEvent(unsigned message, UPInt wParam, UPInt lParam, UPInt hWND, bool preprocess)
{
	if (preprocess)
	{
		Scaleform::GFx::IMEWin32Event ev(Scaleform::GFx::IMEEvent::IME_PreProcessKeyboard, hWND, message, wParam, lParam, 0);
		if (m_pCurrentMovie)
		{
			Scaleform::UInt32 handleEvtRetVal = m_pCurrentMovie->HandleEvent(ev);
			return (handleEvtRetVal& Scaleform::GFx::Movie::HE_NoDefaultAction) != 0;
		}
		return Scaleform::GFx::Movie::HE_NotHandled;
	}
	Scaleform::GFx::IMEWin32Event ev(Scaleform::GFx::IMEEvent::IME_Default, hWND, message, wParam, lParam, true);
	if (m_pCurrentMovie)
	{
		Scaleform::UInt32 handleEvtRetVal = m_pCurrentMovie->HandleEvent(ev);
		return (handleEvtRetVal& Scaleform::GFx::Movie::HE_NoDefaultAction) != 0;
	}
	return Scaleform::GFx::Movie::HE_NotHandled;
}

void GImeHelper::SetImeFocus(GFxMovieView* pMovie, bool bSet)
{
	if (bSet && pMovie)
	{
		SetActiveMovie(pMovie);
		m_pCurrentMovie = pMovie;
		m_pCurrentMovie->HandleEvent(Scaleform::GFx::SetFocusEvent());
	}
	else if (!bSet && pMovie == m_pCurrentMovie)
	{
		m_pCurrentMovie = NULL;
	}
}

	#if CRY_PLATFORM_WINDOWS
// Implement IWindowMessageHandler::PreprocessMessage
void GImeHelper::PreprocessMessage(CRY_HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL imeUIMsg = ImmIsUIMessage(NULL, uMsg, wParam, lParam);
	if ((uMsg == WM_KEYDOWN) || (uMsg == WM_KEYUP) || imeUIMsg || (uMsg == WM_LBUTTONDOWN) || (uMsg == WM_LBUTTONUP))
	{
		OnIMEEvent(uMsg, wParam, lParam, (UPInt)hWnd, true);
	}
}

// Implement IWindowMessageHandler::HandleMessage
bool GImeHelper::HandleMessage(CRY_HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{

	switch (uMsg)
	{
	case WM_IME_STARTCOMPOSITION:
	case WM_IME_KEYDOWN:
	case WM_IME_COMPOSITION:
	case WM_IME_ENDCOMPOSITION:
	case WM_IME_NOTIFY:
	case WM_INPUTLANGCHANGE:
	case WM_IME_CHAR:

		if (OnIMEEvent(uMsg, wParam, lParam, (UPInt)hWnd, false))
		{
			*pResult = 0;
			return true;
		}
		break;
	}
	return false;
}
	#endif // CRY_PLATFORM_WINDOWS
}        // ~Scaleform4 namespace
}        // ~Cry namespace

#endif   //defined(INCLUDE_SCALEFORM_SDK) && defined(USE_GFX_IME)
