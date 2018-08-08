// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QAdvancedTreeView.h>
#include <SharedData.h>

namespace ACE
{
namespace Impl
{
namespace Adx2
{
class CTreeView final : public QAdvancedTreeView
{
public:

	CTreeView() = delete;

	explicit CTreeView(QWidget* const pParent, QAdvancedTreeView::BehaviorFlags const flags = QAdvancedTreeView::BehaviorFlags(UseItemModelAttribute));

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

	int             m_nameRole = 0;
	int             m_nameColumn = 0;
	QSet<ControlId> m_expandedBackup;
	QSet<ControlId> m_selectionBackup;
};
} // namespace Adx2
} // namespace Impl
} // namespace ACE
