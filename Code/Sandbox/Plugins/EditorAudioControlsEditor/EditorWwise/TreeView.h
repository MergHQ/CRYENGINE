// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../Common/SharedData.h"
#include <QAdvancedTreeView.h>

namespace ACE
{
namespace Impl
{
namespace Wwise
{
class CTreeView final : public QAdvancedTreeView
{
public:

	CTreeView() = delete;
	CTreeView(CTreeView const&) = delete;
	CTreeView(CTreeView&&) = delete;
	CTreeView& operator=(CTreeView const&) = delete;
	CTreeView& operator=(CTreeView&&) = delete;

	explicit CTreeView(QWidget* const pParent, QAdvancedTreeView::BehaviorFlags const flags = QAdvancedTreeView::BehaviorFlags(UseItemModelAttribute));
	virtual ~CTreeView() override = default;

	void ExpandSelection();
	void CollapseSelection();

	void BackupExpanded();
	void RestoreExpanded();

	void BackupSelection();
	void RestoreSelection();

	void SetNameRole(int const nameRole)     { m_nameRole = nameRole; }
	void SetNameColumn(int const nameColumn) { m_nameColumn = nameColumn; }

private:

	ControlId GetItemId(QModelIndex const& index) const;

	void      ExpandSelectionRecursively(QModelIndexList const& indexList);
	void      CollapseSelectionRecursively(QModelIndexList const& indexList);

	void      BackupExpandedRecursively(QModelIndex const& index);
	void      RestoreExpandedRecursively(QModelIndex const& index);
	void      RestoreSelectionRecursively(QModelIndex const& index);

	int             m_nameRole;
	int             m_nameColumn;
	QSet<ControlId> m_expandedBackup;
	QSet<ControlId> m_selectionBackup;
};
} // namespace Wwise
} // namespace Impl
} // namespace ACE
