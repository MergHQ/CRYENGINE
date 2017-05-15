// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GraphViewModel.h"

#include "GraphNodeItem.h"
#include "GraphConnectionItem.h"

#include "NodeGraphClipboard.h"

#include <NodeGraph/NodeGraphUndo.h>

#include <Schematyc/Script/IScriptGraph.h>
#include <Schematyc/Script/IScriptRegistry.h>

namespace CrySchematycEditor {

CNodeGraphViewModel::CNodeGraphViewModel(Schematyc::IScriptGraph& scriptGraph /*, CrySchematyc::CNodeGraphRuntimeContext& context*/)
	: m_scriptGraph(scriptGraph)
	, m_runtimeContext(scriptGraph)
{
	{
		auto visitor = [this](Schematyc::IScriptGraphNode& scriptGraphNode) -> Schematyc::EVisitStatus
		{
			CNodeItem* pNodeItem = new CNodeItem(scriptGraphNode, *this);
			m_nodesByIndex.push_back(pNodeItem);
			m_nodesByGuid.emplace(pNodeItem->GetGUID(), pNodeItem);

			return Schematyc::EVisitStatus::Continue;
		};
		scriptGraph.VisitNodes(Schematyc::ScriptGraphNodeVisitor::FromLambda(visitor));
	}

	{
		auto visitor = [this](Schematyc::IScriptGraphLink& scriptGraphLink) -> Schematyc::EVisitStatus
		{
			CNodeItem* pSrcNodeItem = m_nodesByGuid[scriptGraphLink.GetSrcNodeGUID()];
			CNodeItem* pDstNodeItem = m_nodesByGuid[scriptGraphLink.GetDstNodeGUID()];

			if (pSrcNodeItem && pDstNodeItem)
			{
				CPinItem* pSrcPinItem = pSrcNodeItem->GetPinItemById(CPinId(scriptGraphLink.GetSrcOutputId(), CPinId::EType::Output));
				CPinItem* pDstPinItem = pDstNodeItem->GetPinItemById(CPinId(scriptGraphLink.GetDstInputId(), CPinId::EType::Input));

				if (pSrcPinItem == nullptr || pSrcPinItem->IsInputPin() || pDstPinItem == nullptr || pDstPinItem->IsOutputPin())
					return Schematyc::EVisitStatus::Continue;

				if (pSrcNodeItem && pDstPinItem)
				{
					CConnectionItem* pConnectionItem = new CConnectionItem(scriptGraphLink, *pSrcPinItem, *pDstPinItem, *this);
					m_connectionsByIndex.push_back(pConnectionItem);
				}
			}

			return Schematyc::EVisitStatus::Continue;
		};
		scriptGraph.VisitLinks(Schematyc::ScriptGraphLinkVisitor::FromLambda(visitor));
	}
}

CNodeGraphViewModel::~CNodeGraphViewModel()
{
	for (CConnectionItem* pConnectionitem : m_connectionsByIndex)
	{
		delete pConnectionitem;
	}

	for (CNodeItem* pNodeItem : m_nodesByIndex)
	{
		delete pNodeItem;
	}
}

uint32 CNodeGraphViewModel::GetNodeItemCount() const
{
	return m_scriptGraph.GetNodeCount();
}

CryGraphEditor::CAbstractNodeItem* CNodeGraphViewModel::GetNodeItemByIndex(uint32 index) const
{
	if (index < m_nodesByIndex.size())
	{
		return m_nodesByIndex[index];
	}
	return nullptr;
}

CryGraphEditor::CAbstractNodeItem* CNodeGraphViewModel::GetNodeItemById(QVariant id) const
{
	const Schematyc::SGUID guid = id.value<Schematyc::SGUID>();
	for (CNodeItem* pNodeItem : m_nodesByIndex)
	{
		if (pNodeItem->GetGUID() == guid)
			return pNodeItem;
	}

	return nullptr;
}

CryGraphEditor::CAbstractNodeItem* CNodeGraphViewModel::CreateNode(QVariant typeId, const QPointF& position)
{
	Schematyc::IScriptGraphNodeCreationCommand* pCommand = reinterpret_cast<Schematyc::IScriptGraphNodeCreationCommand*>(typeId.value<quintptr>());
	if (pCommand)
	{
		Schematyc::IScriptGraphNodePtr pScriptNode = pCommand->Execute(Vec2(position.x(), position.y()));
		if (pScriptNode && m_scriptGraph.AddNode(pScriptNode))
		{
			// TODO: We shouldn't need to do this here.
			pScriptNode->ProcessEvent(Schematyc::SScriptEvent(Schematyc::EScriptEventId::EditorAdd));
			// ~TODO

			CNodeItem* pNodeItem = new CNodeItem(*pScriptNode, *this);
			m_nodesByIndex.push_back(pNodeItem);

			// TODO: Move this into a CNodeGraphViewModel method that gets called from here.
			SignalCreateNode(*pNodeItem);

			if (GetIEditor()->GetIUndoManager()->IsUndoRecording())
			{
				CUndo::Record(new CryGraphEditor::CUndoNodeCreate(*pNodeItem));
			}
			// ~TODO

			return pNodeItem;
		}
	}
	else
	{
		CryGraphEditor::CAbstractNodeItem* pNodeItem = CreateNode(typeId.value<Schematyc::SGUID>());
		if (pNodeItem)
		{
			pNodeItem->SetPosition(position);
			return pNodeItem;
		}
	}

	return nullptr;
}

bool CNodeGraphViewModel::RemoveNode(CryGraphEditor::CAbstractNodeItem& node)
{
	CNodeItem* pNodeItem = static_cast<CNodeItem*>(&node);
	for (CryGraphEditor::CAbstractPinItem* pPin : pNodeItem->GetPinItems())
	{
		for (CryGraphEditor::CAbstractConnectionItem* pConnection : pPin->GetConnectionItems())
		{
			RemoveConnection(*pConnection);
		}
	}

	NodesByIndex::iterator result = std::find(m_nodesByIndex.begin(), m_nodesByIndex.end(), pNodeItem);
	if (result != m_nodesByIndex.end())
	{
		// TODO: Move this into a CNodeGraphViewModel method that gets called from here.
		if (GetIEditor()->GetIUndoManager()->IsUndoRecording())
		{
			CUndo::Record(new CryGraphEditor::CUndoNodeRemove(node));
		}

		SignalRemoveNode(*pNodeItem);
		// ~TODO
		m_nodesByIndex.erase(result);
		m_scriptGraph.RemoveNode(pNodeItem->GetGUID());

		delete pNodeItem;

		return true;
	}

	m_scriptGraph.RemoveNode(pNodeItem->GetGUID());
	return false;
}

uint32 CNodeGraphViewModel::GetConnectionItemCount() const
{
	return m_connectionsByIndex.size();
}

CryGraphEditor::CAbstractConnectionItem* CNodeGraphViewModel::GetConnectionItemByIndex(uint32 index) const
{
	if (index < m_connectionsByIndex.size())
	{
		return m_connectionsByIndex[index];
	}
	return nullptr;
}

CryGraphEditor::CAbstractConnectionItem* CNodeGraphViewModel::GetConnectionItemById(QVariant id) const
{
	const Schematyc::IScriptGraphLink* pGraphLink = reinterpret_cast<Schematyc::IScriptGraphLink*>(id.value<quintptr>());
	for (CConnectionItem* pConnection : m_connectionsByIndex)
	{
		if (&pConnection->GetScriptLink() == pGraphLink)
		{
			return pConnection;
		}
	}
	return nullptr;
}

CryGraphEditor::CAbstractConnectionItem* CNodeGraphViewModel::CreateConnection(CryGraphEditor::CAbstractPinItem& sourcePin, CryGraphEditor::CAbstractPinItem& targetPin)
{
	CPinItem& sourcePinItem = static_cast<CPinItem&>(sourcePin);
	CPinItem& targetPinItem = static_cast<CPinItem&>(targetPin);

	CNodeItem& sourceNode = static_cast<CNodeItem&>(sourcePinItem.GetNodeItem());
	CNodeItem& targetNode = static_cast<CNodeItem&>(targetPinItem.GetNodeItem());

	if (sourcePinItem.CanConnect(&targetPinItem))
	{
		Schematyc::IScriptGraphLink* pScriptLink = m_scriptGraph.AddLink(sourceNode.GetGUID(), sourcePinItem.GetPortId(), targetNode.GetGUID(), targetPinItem.GetPortId());
		if (pScriptLink)
		{
			CConnectionItem* pConnectionItem = new CConnectionItem(*pScriptLink, sourcePinItem, targetPinItem, *this);
			m_connectionsByIndex.push_back(pConnectionItem);

			gEnv->pSchematyc->GetScriptRegistry().ElementModified(m_scriptGraph.GetElement());

			// TODO: Move this into a CNodeGraphViewModel method that gets called from here.
			SignalCreateConnection(*pConnectionItem);

			if (GetIEditor()->GetIUndoManager()->IsUndoRecording())
			{
				CUndo::Record(new CryGraphEditor::CUndoConnectionCreate(*pConnectionItem));
			}
			// ~TODO

			return pConnectionItem;
		}
	}

	return nullptr;
}

bool CNodeGraphViewModel::RemoveConnection(CryGraphEditor::CAbstractConnectionItem& connection)
{
	CConnectionItem* pConnectionItem = static_cast<CConnectionItem*>(&connection);

	CPinItem& sourcePinItem = static_cast<CPinItem&>(connection.GetSourcePinItem());
	CPinItem& targetPinItem = static_cast<CPinItem&>(connection.GetTargetPinItem());

	CNodeItem& sourceNode = static_cast<CNodeItem&>(sourcePinItem.GetNodeItem());
	CNodeItem& targetNode = static_cast<CNodeItem&>(targetPinItem.GetNodeItem());

	const uint32 linkIndex = m_scriptGraph.FindLink(sourceNode.GetGUID(), sourcePinItem.GetPortId(), targetNode.GetGUID(), targetPinItem.GetPortId());
	if (linkIndex != Schematyc::InvalidIdx)
	{
		const ConnectionsByIndex::iterator result = std::find(m_connectionsByIndex.begin(), m_connectionsByIndex.end(), &connection);
		if (result != m_connectionsByIndex.end())
		{
			// TODO: Move this into a CNodeGraphViewModel method that gets called from here.
			if (GetIEditor()->GetIUndoManager()->IsUndoRecording())
			{
				CUndo::Record(new CryGraphEditor::CUndoConnectionRemove(connection));
			}

			SignalRemoveConnection(connection);
			// ~TODO

			m_connectionsByIndex.erase(result);
			m_scriptGraph.RemoveLink(linkIndex);

			delete pConnectionItem;

			return true;
		}

		m_scriptGraph.RemoveLink(linkIndex);
	}

	return false;
}

CryGraphEditor::CItemCollection* CNodeGraphViewModel::CreateClipboardItemsCollection()
{
	return new CNodeGraphClipboard(*this);
}

CNodeItem* CNodeGraphViewModel::CreateNode(Schematyc::SGUID typeGuid)
{
	Schematyc::IScriptGraphNodePtr pScriptNode = m_scriptGraph.AddNode(typeGuid);
	if (pScriptNode)
	{
		// TODO: This should happen in backend!
		pScriptNode->ProcessEvent(Schematyc::SScriptEvent(Schematyc::EScriptEventId::EditorAdd));
		// ~TODO

		CNodeItem* pNodeItem = new CNodeItem(*pScriptNode, *this);
		m_nodesByIndex.push_back(pNodeItem);

		// TODO: Move this into a CNodeGraphViewModel method that gets called from here.
		SignalCreateNode(*pNodeItem);

		if (GetIEditor()->GetIUndoManager()->IsUndoRecording())
		{
			CUndo::Record(new CryGraphEditor::CUndoNodeCreate(*pNodeItem));
		}
		// ~TODO
		return pNodeItem;
	}
	return nullptr;
}

void CNodeGraphViewModel::Refresh()
{
	for (CNodeItem* pNode : m_nodesByIndex)
	{
		pNode->Refresh();
	}
}

}
