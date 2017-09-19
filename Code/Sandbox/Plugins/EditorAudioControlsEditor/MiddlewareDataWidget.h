// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QFrame>
#include <ACETypes.h>

class CElidedLabel;

namespace ACE
{
class CMiddlewareDataFilterProxyModel;
class CMiddlewareDataModel;
class CAdvancedTreeView;

class CMiddlewareDataWidget final : public QFrame
{
	Q_OBJECT

public:

	CMiddlewareDataWidget();
	virtual ~CMiddlewareDataWidget() override;

	void SetAllowedControls(EItemType type, bool bAllowed);
	void Reset();
	void BackupTreeViewStates();
	void RestoreTreeViewStates();

private slots:

	void OnContextMenu(QPoint const& pos) const;

private:

	bool                          m_allowedATLTypes[EItemType::eItemType_NumTypes];
	CMiddlewareDataFilterProxyModel* m_pModelProxy;
	CMiddlewareDataModel*            m_pModel;
	CElidedLabel*                 m_pImplNameLabel;
	CAdvancedTreeView*            m_pTreeView;
	QString                       m_filter;
};
} // namespace ACE
