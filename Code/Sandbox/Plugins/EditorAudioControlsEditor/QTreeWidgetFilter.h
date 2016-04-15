// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class QTreeWidget;
class QTreeWidgetItem;

struct ITreeWidgetItemFilter
{
	virtual bool IsItemValid(QTreeWidgetItem* pItem) = 0;
};

class QTreeWidgetFilter
{

public:
	QTreeWidgetFilter();
	void SetTree(QTreeWidget* pTreeWidget);
	void AddFilter(ITreeWidgetItemFilter* filter);
	void ApplyFilter();

private:
	bool IsItemValid(QTreeWidgetItem* pItem);

	QTreeWidget*                        m_pTreeWidget;
	std::vector<ITreeWidgetItemFilter*> m_filters;
};
