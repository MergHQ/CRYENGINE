// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include <QSortFilterProxyModel>

class CAdvancedFileSystemModel;

#pragma warning(push)
// 'setSourceModel': no override available for virtual member function from base 'QSortFilterProxyModel'; function is hidden
#pragma warning(disable:4264)

/**
 * \brief specialized filter that filters out all files
 * \note used for the directory column
 */
class CDirectoriesOnlyProxyModel
	: public QSortFilterProxyModel
{
	// as we do not need registered QObject behavior it's not registered as such
public:
	explicit CDirectoriesOnlyProxyModel(QObject* parent = nullptr);

	// QSortFilterProxyModel interface
protected:
	virtual bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const;

public:

	// Hiding base virtual is intended
	virtual void setSourceModel(CAdvancedFileSystemModel* sourceFileSystem);
};
#pragma warning(pop)
