// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QAdvancedTreeView.h>

namespace ACE
{
class CTreeView final : public QAdvancedTreeView
{
	Q_OBJECT

public:

	CTreeView() = delete;
	CTreeView(CTreeView const&) = delete;
	CTreeView(CTreeView&&) = delete;
	CTreeView& operator=(CTreeView const&) = delete;
	CTreeView& operator=(CTreeView&&) = delete;

	explicit CTreeView(QWidget* pParent, QAdvancedTreeView::BehaviorFlags flags = QAdvancedTreeView::BehaviorFlags(UseItemModelAttribute));
	virtual ~CTreeView() override = default;

	bool IsEditing() const { return state() == QAbstractItemView::EditingState; }

	void ExpandSelection();
	void CollapseSelection();

	void BackupExpanded();
	void RestoreExpanded();

	void BackupSelection();
	void RestoreSelection();

	void SetNameRole(int nameRole)     { m_nameRole = nameRole; }
	void SetTypeRole(int typeRole)     { m_typeRole = typeRole; }
	void SetNameColumn(int nameColumn) { m_nameColumn = nameColumn; }

private:

	uint32 GetItemId(QModelIndex const& index) const;

	void   ExpandSelectionRecursively(QModelIndexList const& indexList);
	void   CollapseSelectionRecursively(QModelIndexList const& indexList);

	void   BackupExpandedRecursively(QModelIndex const& index);
	void   RestoreExpandedRecursively(QModelIndex const& index);
	void   RestoreSelectionRecursively(QModelIndex const& index);

	int          m_nameRole;
	int          m_typeRole;
	int          m_nameColumn;
	QSet<uint32> m_expandedBackup;
	QSet<uint32> m_selectionBackup;
};
} // namespace ACE
