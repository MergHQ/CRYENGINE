// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "MannequinBase.h"

class CMannequinModelViewport;
class CPropertiesPanel;

class CMannequinDebugOptionsDialog : public CXTResizeDialog
{
public:
	CMannequinDebugOptionsDialog(CMannequinModelViewport* pModelViewport, CWnd* pParent);

protected:
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();

	std::unique_ptr<CPropertiesPanel> m_pPanel;
	CMannequinModelViewport*          m_pModelViewport;
};
