// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorFramework/Editor.h"
#include "CSharpEditorPlugin.h"
#include "CSharpOutputModel.h"

class QFilteringPanel;
class QAdvancedTreeView;
class QAttributeFilterProxyModel;

//! Window designed to show compile messages from the CompiledMonoLibrary.
class CCSharpOutputWindow final
	: public CDockableEditor
	  , public ICSharpMessageListener
{
public:
	CCSharpOutputWindow(QWidget* pParent = nullptr);
	virtual ~CCSharpOutputWindow();

	// CDockableEditor
	virtual const char* GetEditorName() const override final { return "C# Output"; }
	virtual void        SetLayout(const QVariantMap& state) override final;
	virtual QVariantMap GetLayout() const override final;
	// ~CDockableEditor

	// ICSharpMessageListener
	virtual void OnMessagesUpdated() override final;
	// ~ICSharpMessageListener

	QAdvancedTreeView* GetView() const;

private:
	QAdvancedTreeView*          m_pDetailsView;
	CCSharpOutputModel*         m_pModel;
	QAttributeFilterProxyModel* m_pFilter;
	QFilteringPanel*            m_pFilteringPanel;

	void OnDoubleClick(const QModelIndex& index);
};