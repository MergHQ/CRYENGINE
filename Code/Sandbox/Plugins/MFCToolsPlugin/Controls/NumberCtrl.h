// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "MFCToolsDefines.h"
#include "NumberCtrlEdit.h"

// Sent to parent when user start to change control value.
#define EN_BEGIN_DRAG EN_CHANGE + 0x1000
// Sent to parent when user stops to change control value.
#define EN_END_DRAG   EN_CHANGE + 0x1001

class MFC_TOOLS_PLUGIN_API CNumberCtrl : public CWnd
{
public:
	typedef Functor1<CNumberCtrl*> UpdateCallback;
	enum Flags
	{
		LEFTARROW    = 0x01, //!< Place arrows at left side of edit control.
		NOBORDER     = 0x02, //!< Not draw border around edit control.
		LEFTALIGN    = 0x04, //!< Align text to left side.
		CENTER_ALIGN = 0x08, //!< Align text to center.
	};

	CNumberCtrl();
	virtual ~CNumberCtrl();

	void Create(CWnd* parentWnd, UINT ctrlID, int nFlags = 0);
	void Create(CWnd* parentWnd, CRect& rc, UINT nID, int nFlags = 0);

	//! If called will enable undo with given text when control is modified.
	void EnableUndo(const CString& undoText);

	void SetUpdateCallback(const UpdateCallback& cb) { m_updateCallback = cb; }

	void EnableNotifyWithoutValueChange(bool bFlag);

	void SetMultiplier(double fMultiplier);

	//! Set/get current value.
	void    SetValue(double val);
	double  GetValue() const;
	CString GetValueAsString() const;

	//! Set/get increment step.
	void   SetStep(double step);
	double GetStep() const;

	//! Set min/max values.
	void SetRange(double min, double max);

	//! Value in control will be integer.
	void SetInteger(bool enable);
	//! If left is true align text in edit control to left, otherwise to right.
	void SetLeftAlign(bool left);

	//! set the internal precision for floats m.m. (default is 2)
	void SetInternalPrecision(int iniDigits);

	//! Overridden from MFC.
	void SetFont(CFont* pFont, BOOL bRedraw = TRUE);

	void SetFloatFormatPrecision(int significantDigits);

	//! Swallows RETURN key input when editing on the edit control
	void SetSwallowReturn(bool bDoSwallow);

	// Generated message map functions
protected:
	void         DrawButtons(CDC& dc);
	//{{AFX_MSG(CNumberCtrl)
	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnEnable(BOOL bEnable);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnEditSetFocus();
	afx_msg void OnEditKillFocus();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	void         GetBtnRect(int btn, CRect& rc);
	int          GetBtn(CPoint point);
	void         SetBtnStatus(int s);
	void         NotifyUpdate(bool tracking, bool changed);
	void         OnEditChanged();
	void         LoadIcons();

	void         SetInternalValue(double val);
	//! Get current value.
	double       GetInternalValue() const;

	CNumberCtrlEdit m_edit;

	int             m_nFlags;
	double          m_step;
	double          m_min, m_max;
	mutable double  m_value;
	double          m_lastUpdateValue;
	double          m_multiplier;
	// 0 if no buttons pressed.
	// 1 if up button pressed.
	// 2 if down button pressed.
	int            m_btnStatus;
	int            m_btnWidth;
	CPoint         m_mousePos;
	bool           m_draggin;
	HICON          m_upArrow, m_downArrow;
	HCURSOR        m_upDownCursor;
	bool           m_enabled;
	bool           m_noNotify;
	bool           m_integer;

	bool           m_bUndoEnabled;
	bool           m_bDragged;
	bool           m_bLocked; // When locked, not accept SetValue.
	bool           m_bInitialized;
	bool           m_bBlackArrows;

	bool           m_bInitializedValue;
	bool           m_bForceModified;

	CString        m_undoText;
	int            m_floatFormatPrecision;

	UpdateCallback m_updateCallback;

	//! calculate the digits right from the comma
	//! \param infNumber source double number
	//! \param iniMaxPlaces maximum number of places (used for numeric rounding problems)
	//! \return number of places 0..iniMaxPlaces
	static int   CalculateDecimalPlaces(double infNumber, int iniMaxPlaces);
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
