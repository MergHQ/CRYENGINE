// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/SharedData.h"
#include <EditorFramework/EditorWidget.h>

class QAttributeFilterProxyModel;
class QSearchBox;

namespace ACE
{
class CContextModel;
class CTreeView;

class CContextWidget final : public CEditorWidget
{
	Q_OBJECT

public:

	CContextWidget() = delete;
	CContextWidget(CContextWidget const&) = delete;
	CContextWidget(CContextWidget&&) = delete;
	CContextWidget& operator=(CContextWidget const&) = delete;
	CContextWidget& operator=(CContextWidget&&) = delete;

	explicit CContextWidget(QWidget* const pParent);
	virtual ~CContextWidget() override;

	void Reset();
	void OnBeforeReload();
	void OnAfterReload();

private slots:

	void OnContextMenu(QPoint const& pos);
	void OnItemDoubleClicked(QModelIndex const& index);

private:

	void ActivateContext(CryAudio::ContextId const contextId);
	void DeactivateContext(CryAudio::ContextId const contextId);
	void ActivateMultipleContexts();
	void DeactivateMultipleContexts();
	void AddContext();
	void RenameContext();
	bool DeleteSelectedContexts();
	void SelectNewContext(QModelIndex const& parent, int const row);

	QSearchBox* const                 m_pSearchBox;
	QAttributeFilterProxyModel* const m_pAttributeFilterProxyModel;
	CContextModel* const              m_pContextModel;
	CTreeView* const                  m_pTreeView;
};
} // namespace ACE
