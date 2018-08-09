// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "QDialog"
#include "VersionControl/VersionControlFileConflictStatus.h"

class QTreeView;

class EDITOR_COMMON_API CResolveConflictsDialog : public QDialog
{
	Q_OBJECT
public:
	CResolveConflictsDialog(const QStringList& conflictFiles, QWidget* parent = nullptr);

signals:
	void resolve(const std::vector<SVersionControlFileConflictStatus>& conflictFiles);

public slots:
	void onButtonClicked();

private:
	std::vector<SVersionControlFileConflictStatus> m_conflictFiles;
	QTreeView* m_pFileListView;
};
