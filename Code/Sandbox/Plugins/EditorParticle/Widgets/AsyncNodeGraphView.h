// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <NodeGraph/NodeGraphView.h>
#include <Notifications/NotificationCenter.h>

// The implementation of CryGraphEditor::CNodeGraphView, which allows you to avoid blocking the user interface for a long time when building the graph view.
class CAsyncNodeGraphView : public CryGraphEditor::CNodeGraphView
{
public:
	CAsyncNodeGraphView();

	virtual void ReloadItems() override;

	void         UpdateProgressNotification(const size_t itemsCountToProcess, const size_t totalItemsCount);

private:
	uint32 m_reloadItemsId;
	std::unique_ptr<CProgressNotification> m_pProgressNotification;
};
