// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "MFCToolsDefines.h"
#include "ColorCtrl.h"

// hide warnings about hides CXTPButton::SetIcon
#pragma warning(push)
#pragma warning(disable:4264)
class MFC_TOOLS_PLUGIN_API CCustomButton : public CXTPButton
{
public:
	CCustomButton();
	//! Set icon to display on button, (must not be shared).
	void SetIcon(HICON hIcon, int nIconAlign = BS_CENTER, bool bDrawText = false);
	void SetIcon(LPCSTR lpszIconResource, int nIconAlign = BS_CENTER, bool bDrawText = false);
	void SetToolTip(const char* tooltipText);
};
#pragma warning(pop)
typedef CCustomButton CColoredPushButton;
