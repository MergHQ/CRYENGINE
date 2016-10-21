// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_GraphViewModel.h"

#include "Schematyc_GraphNodeItem.h"
#include "Schematyc_GraphConnectionItem.h"

#include <Schematyc/Script/Schematyc_IScriptGraph.h>
#include <Schematyc/Script/Schematyc_IScriptRegistry.h>

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
	Schematyc::IScriptGraphNodeCreationMenuCommand* pCommand = reinterpret_cast<Schematyc::IScriptGraphNodeCreationMenuCommand*>(typeId.value<quintptr>());

	Schematyc::IScriptGraphNodePtr pScriptNode = pCommand->Execute(Vec2(position.x(), position.y()));
	if (pScriptNode && m_scriptGraph.AddNode(pScriptNode))
	{
		pScriptNode->ProcessEvent(Schematyc::SScriptEvent(Schematyc::EScriptEventId::EditorAdd));

		CNodeItem* pNodeItem = new CNodeItem(*pScriptNode, *this);
		m_nodesByIndex.push_back(pNodeItem);

		SignalCreateNode(*pNodeItem);
		return pNodeItem;
	}

	return nullptr;
}

bool CNodeGraphViewModel::RemoveNode(CryGraphEditor::CAbstractNodeItem& node)
{
	CNodeItem& nodeItem = static_cast<CNodeItem&>(node);
	for (CryGraphEditor::CAbstractPinItem* pPin : nodeItem.GetPinItems())
	{
		for (CryGraphEditor::CAbstractConnectionItem* pConnection : pPin->GetConnectionItems())
		{
			RemoveConnection(*pConnection);
		}
	}

	NodesByIndex::iterator result = std::find(m_nodesByIndex.begin(), m_nodesByIndex.end(), &nodeItem);
	if (result != m_nodesByIndex.end())
	{
		SignalRemoveNode(nodeItem);
		m_nodesByIndex.erase(result);
		m_scriptGraph.RemoveNode(nodeItem.GetGUID());

		return true;
	}

	m_scriptGraph.RemoveNode(nodeItem.GetGUID());
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
	return reinterpret_cast<CryGraphEditor::CAbstractConnectionItem*>(id.value<quintptr>());
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

			GetSchematycFramework().GetScriptRegistry().ElementModified(m_scriptGraph.GetElement());

			SignalCreateConnection(*pConnectionItem);

			return pConnectionItem;
		}
	}

	return nullptr;
}

bool CNodeGraphViewModel::RemoveConnection(CryGraphEditor::CAbstractConnectionItem& connection)
{
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
			SignalRemoveConnection(connection);
			m_connectionsByIndex.erase(result);
			m_scriptGraph.RemoveLink(linkIndex);

			return true;
		}

		m_scriptGraph.RemoveLink(linkIndex);
	}

	return false;
}

}
