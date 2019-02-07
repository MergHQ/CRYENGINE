// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "QWidget"
#include "EditorCommonAPI.h"
#include <CrySandbox/CrySignal.h>

class QTreeView;
class CPendingChange;
class CAsset;

class EDITOR_COMMON_API CVersionControlPendingChangesWidget : public QWidget
{
	Q_OBJECT
public:
	explicit CVersionControlPendingChangesWidget(QWidget* pParent);

	~CVersionControlPendingChangesWidget();

	//! Marks given assets as selected if they can be submitted.
	//! \param shouldDeselectCurrent specifies if the current selected item need to be cleared.
	void SelectAssets(const std::vector<CAsset*>& assets, bool shouldDeselectCurrent = true);

	//! Marks given layers as selected if they can be submitted.
	//! \param shouldDeselectCurrent specifies if the current selected item need to be cleared.
	void SelectLayers(const std::vector<string>& layersFiles, bool shouldDeselectCurrent = true);

	//! Marks all items in given folders as selected if they can be submitted.
	//! \param shouldDeselectCurrent specifies if the current selected item need to be cleared.
	void SelectFolders(const std::vector<string>& folders, bool shouldDeselectCurrent);

	//! Returns the list selected of pending changes.
	std::vector<CPendingChange*> GetSelectedPendingChanges() const;

	CCrySignal<void()> signalSelectionChanged;

private:
	QTreeView* m_pTree{ nullptr };
};
