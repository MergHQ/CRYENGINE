// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Controls/ColorCheckBox.h"
#include <Util/Image.h>

class CTerrainTextureExport : public CDialog
{
public:
	CTerrainTextureExport(CWnd* pParent = nullptr);
	~CTerrainTextureExport();

	enum { IDD = IDD_TERRAIN_TEXTURE_EXPORT };

	void ImportExport(bool bIsImport, bool bIsClipboard = false);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	string       BrowseTerrainTexture(bool bIsSave);

	void         DrawPreview(CDC& dc);
	void         PreviewToTile(uint32& outX, uint32& outY, uint32 x, uint32 y);

protected:
	CBitmap               m_bmpLightmap;
	CDC                   m_dcLightmap;
	class CTerrainTexGen* m_pTexGen;
	CImageEx              m_terrain;

	CCustomButton         m_importBtn;
	CCustomButton         m_exportBtn;
	CCustomButton         m_closeBtn;
	CCustomButton         m_changeResolutionBtn;

	int                   m_cx;
	int                   m_cy;

	bool                  m_bSelectMode;
	RECT                  rcSel;

	// Generated message map functions
	//{{AFX_MSG(CTerrainTextureExport)
	afx_msg void OnPaint();
	afx_msg void OnGenerate();
	virtual BOOL OnInitDialog();
	afx_msg void OnExport();
	afx_msg void OnImport();
	afx_msg void OnChangeResolutionBtn();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
