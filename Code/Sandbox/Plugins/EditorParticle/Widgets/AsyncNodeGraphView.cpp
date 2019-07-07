// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AsyncNodeGraphView.h"

#include <NodeGraph/AbstractNodeGraphViewModel.h>

#include <CryCore/CryCrc32.h>

namespace Private_AsyncNodeGraphView
{

// A utility class for simulating a cooperative multitasking environment for long lasting functions executing.
template <int idleTimeFrameMsec, typename... Types>
class CAutoResume final
{
public:
	CAutoResume(std::function<void()> function, Types&... args)
		: m_function(function)
		, m_context(args...)
	{
		if (s_pContext)
		{
			m_context = *s_pContext;
		}
	}

	~CAutoResume()
	{
		if (m_completed)
		{
			return;
		}

		std::function<void()> function = std::move(m_function);
		std::tuple<Types...> context = m_context;
		QTimer::singleShot(idleTimeFrameMsec, [function, context]()
		{
			s_pContext = &context;
			function();
			s_pContext = nullptr;
		});
	}

	bool isStarting() { return !s_pContext; }
	void SetComplete() { m_completed = true; }

	void Restart()
	{
		SetComplete();

		std::function<void()> function = std::move(m_function);
		QTimer::singleShot(0, [function]()
		{
			function();
		});
	}

private:
	std::function<void()> m_function;
	std::tuple<Types&...> m_context;
	bool m_completed = false;
	static const std::tuple<Types...>* s_pContext;
};

template <int idleTimeFrameMsec, typename... Types>
const std::tuple<Types...>* CAutoResume<idleTimeFrameMsec, Types...>::s_pContext = nullptr;

class CAutoFunction final
{
public:
	CAutoFunction(std::function<void()> f)
		: m_function(f)
	{
	}
	~CAutoFunction()
	{
		m_function();
	}
protected:
	std::function<void()> m_function;
};

class CPeriod final
{
public:
	CPeriod(float seconds)
		: m_endTime(GetISystem()->GetITimer()->GetAsyncCurTime() + seconds)
	{
	}

	bool Expired()
	{
		return m_endTime < GetISystem()->GetITimer()->GetAsyncCurTime();
	}
private:
	const float m_endTime;
};

}

CAsyncNodeGraphView::CAsyncNodeGraphView()
	: m_reloadItemsId(0)
{
}

// Building a graph view can be a time consuming operation that blocks UI. To avoid blocking the UI for a long time 
// this function uses CAutoResume to simulate a cooperative multitasking environment, 
// thus the function execution can be suspended and resumed at certain locations. The function is always resumed in the UI thread.
void CAsyncNodeGraphView::ReloadItems()
{
	using namespace CryGraphEditor;
	using namespace Private_AsyncNodeGraphView;

	CNodeGraphViewModel* const pModel = GetModel();

	// This may be the case if the the model was changed while the coroutine was awaiting of resuming.
	if (!pModel)
	{
		return; // cancel the coroutine.
	}

	// State variables for resuming the function execution.
	int32 nodeIndex = pModel->GetNodeItemCount();
	int32 commentIndex = pModel->GetCommentItemCount();
	int32 connectionIndex = pModel->GetConnectionItemCount();
	int32 groupIndex = pModel->GetGroupItemCount();

	uint32 coroutineId = m_reloadItemsId;

	CCrc32 crc;
	crc.Add(&nodeIndex, sizeof(nodeIndex));
	crc.Add(&commentIndex, sizeof(commentIndex));
	crc.Add(&connectionIndex, sizeof(connectionIndex));
	crc.Add(&groupIndex, sizeof(groupIndex));
	uint32 modelStateId = crc.Get();

	// To the progress notification.
	const auto getItemsCount = [&] { return nodeIndex + commentIndex + connectionIndex + groupIndex; };
	const size_t totalItemsCount = getItemsCount();

	// The view can be closed while the coroutine is awaiting of resuming, therefore we can not use WrapMemberFunction(this, &CNodeGraphView::ReloadItems)
	// Using QPointer we can safely test the view for validity.
	QPointer<CAsyncNodeGraphView> pThisView = this;
	auto thisFunction = [pThisView]()
	{
		if (pThisView)
		{
			pThisView->ReloadItems();
		}
	};

	// Captures and restores the last used state variable values if the function is resumed.
	CAutoResume<0, uint32, uint32, int32, int32, int32, int32> coroutine(thisFunction,
		coroutineId, modelStateId, nodeIndex, commentIndex, connectionIndex, groupIndex);

	if (coroutine.isStarting())
	{
		coroutineId = ++m_reloadItemsId;

		setEnabled(false);

		SetStyle(pModel->GetRuntimeContext().GetStyle());
		DeselectAllItems();
		ClearItems();
	}

	// There was a new call of ReloadItems, either as a result of setting of a new model, 
	// or the model signaled the invalidated state. So we will simply cancel the old one.
	// see CNodeGraphView::SetModel
	if (coroutineId != m_reloadItemsId)
	{
		coroutine.SetComplete();
		return;
	}

	// The model state was changed while the coroutine was awaiting of resuming.
	if (modelStateId != crc.Get())
	{
		// The function cannot continue correctly with changed model.
		// Therefore, we do ReloadItems() from the very beginning.
		coroutine.Restart();
		return;
	}

	// The amount of time after which the function has to yield the control back to the caller.
	CPeriod timeFrame(0.1f);

	CAutoFunction onReturn([this, totalItemsCount, &getItemsCount]()
	{
		const size_t itemsCountToProcess = getItemsCount();
		UpdateProgressNotification(itemsCountToProcess, totalItemsCount);
		this->update();
	});

	while (nodeIndex > 0)
	{
		if (CAbstractNodeItem* pItem = pModel->GetNodeItemByIndex(--nodeIndex))
		{
			AddNodeItem(*pItem);
		}

		if (timeFrame.Expired())
		{
			return; // yield control back to the caller. CAutoResume will resume the function in the future, in accordance to the idleTimeFrameMsec value.
		}
	}

	while (commentIndex > 0)
	{
		if (CAbstractCommentItem* pItem = pModel->GetCommentItemByIndex(--commentIndex))
		{
			AddCommentItem(*pItem);
		}

		if (timeFrame.Expired())
		{
			return;
		}
	}

	while (connectionIndex > 0)
	{
		if (CAbstractConnectionItem* pItem = pModel->GetConnectionItemByIndex(--connectionIndex))
		{
			AddConnectionItem(*pItem);
		}

		if (timeFrame.Expired())
		{
			return;
		}
	}

	// groups should be restored after all other items restored
	while (groupIndex > 0)
	{
		if (CAbstractGroupItem* pItem = pModel->GetGroupItemByIndex(--groupIndex))
		{
			AddGroupItem(*pItem);
		}

		if (timeFrame.Expired())
		{
			return;
		}
	}

	coroutine.SetComplete();
	setEnabled(true);

	SignalItemsReloaded(*this);
}

void CAsyncNodeGraphView::UpdateProgressNotification(const size_t itemsCountToProcess, const size_t totalItemsCount)
{
	if (itemsCountToProcess == 0)
	{
		m_pProgressNotification.reset();
		return;
	}

	if (!m_pProgressNotification)
	{
		m_pProgressNotification.reset(new CProgressNotification(tr("Building graph view"), GetModel()->GetGraphName(), true));
	}

	const float progress = (static_cast<float>(totalItemsCount) - itemsCountToProcess) / totalItemsCount;

	m_pProgressNotification->SetMessage(GetModel()->GetGraphName());
	m_pProgressNotification->SetProgress(progress);
}
