// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "ScaleformTypes.h"

#if defined(INCLUDE_SCALEFORM_SDK) && defined(USE_GFX_IME)
	#include <CrySystem/IWindowMessageHandler.h>

	#if CRY_PLATFORM_WINDOWS
		#include "GFxIME\GFx_IMEManagerWin32.h"
		#include "GFx\GFx_FontProviderWin32.h"
		#include "GFx\GFx_FontProviderHUD.h"

namespace Cry {
namespace Scaleform4 {

typedef Scaleform::GFx::IME::GFxIMEManagerWin32 GFxImeManagerBase;
	#else
		#error No IME implementation on this platform
	#endif

// Helper for Scaleform IME
class GImeHelper : public GFxImeManagerBase, public IWindowMessageHandler
{
public:
	GImeHelper();
	~GImeHelper();
	bool ApplyToLoader(GFxLoader* pLoader);
	bool ForwardEvent(GFxIMEEvent& event);
	void SetImeFocus(GFxMovieView* pMovie, bool bSet);

	#if CRY_PLATFORM_WINDOWS
	// IWindowMessageHandler
	void PreprocessMessage(CRY_HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	bool HandleMessage(CRY_HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	bool OnIMEEvent(unsigned message, UPInt wParam, UPInt lParam, UPInt hWND, bool preprocess);
	// ~IWindowMessageHandler
	#endif

private:
	// Get the current movie
	GFxMovieView* m_pCurrentMovie;
};

} // ~Scaleform4 namespace
} // ~Cry namespace

#endif //defined(INCLUDE_SCALEFORM_SDK) && defined(USE_GFX_IME)
