// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "MFCToolsDefines.h"
#include "ColorButton.h"

/*
   class CXTPButtonNoBorderTheme : public CXTPButtonUltraFlatTheme
   {
   public:
   CXTPButtonNoBorderTheme()
   {
    m_bFlatGlyphs = TRUE;
    m_nBorderWidth = 3;
    m_bHiglightButtons = TRUE;
   }
   void RefreshMetrics(CXTPButton* pButton)
   {
    __super::RefreshMetrics(pButton);
   }
   };
 */

//////////////////////////////////////////////////////////////////////////
class MFC_TOOLS_PLUGIN_API CInPlaceButton : public CXTPButton
{
	DECLARE_DYNAMIC(CInPlaceButton)

public:
	typedef Functor0 OnClick;

	CInPlaceButton(OnClick onClickFunctor);
	CInPlaceButton(OnClick onClickFunctor, bool bNoTheme);

	// Simulate on click.
	void     Click()                    { OnBnClicked(); }

	void     SetColorFace(int clr)      {}

	void     SetColor(COLORREF col)     {}
	void     SetTextColor(COLORREF col) {}
	COLORREF GetColor() const           { return 0; }

protected:
	DECLARE_MESSAGE_MAP()
	int OnCreate(LPCREATESTRUCT lpCreateStruct);

public:
	afx_msg void OnBnClicked()
	{
		if (m_onClick)
			m_onClick();
	}

	OnClick m_onClick;
};

class MFC_TOOLS_PLUGIN_API CInPlaceCheckBox : public CInPlaceButton
{
public:
	CInPlaceCheckBox(CInPlaceButton::OnClick onClickFunctor) : CInPlaceButton(onClickFunctor, true) {}
};

//////////////////////////////////////////////////////////////////////////
class MFC_TOOLS_PLUGIN_API CInPlaceColorButton : public CColorButton
{
	DECLARE_DYNAMIC(CInPlaceColorButton)

public:
	typedef Functor0 OnClick;

	CInPlaceColorButton(OnClick onClickFunctor) { m_onClick = onClickFunctor; }

	// Simuale on click.
	void Click() { OnBnClicked(); }

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

public:
	afx_msg void OnBnClicked()
	{
		if (m_onClick)
			m_onClick();
	}

	OnClick m_onClick;
};
