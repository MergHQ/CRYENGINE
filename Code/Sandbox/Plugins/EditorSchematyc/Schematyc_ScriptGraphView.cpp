// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_ScriptGraphView.h"

#include <QBoxLayout>
#include <QParentWndWidget.h>
#include <QPushButton>
#include <QSplitter>
#include <QPropertyTree/QPropertyTree.h>
#include <CrySerialization/IArchiveHost.h>
#include <CrySystem/ITimer.h>
#include <Schematyc/Schematyc_FundamentalTypes.h>
#include <Schematyc/Env/Schematyc_IEnvRegistry.h>
#include <Schematyc/Reflection/Schematyc_Reflection.h>
#include <Schematyc/SerializationUtils/Schematyc_ISerializationContext.h>
#include <Schematyc/SerializationUtils/Schematyc_IValidatorArchive.h>
#include <Schematyc/Utils/Schematyc_SharedString.h>

#include "Schematyc_QuickSearchDlg.h"
#include "QMFCApp/QWinHost.h"

namespace Schematyc
{
namespace
{
static const struct
{
	EScriptGraphColor color;
	Gdiplus::Color    value;
} g_colorValues[] =
{
	{ EScriptGraphColor::Red,    Gdiplus::Color(215, 55,  55)  },
	{ EScriptGraphColor::Green,  Gdiplus::Color(38,  184, 33)  },
	{ EScriptGraphColor::Blue,   Gdiplus::Color(0,   108, 217) },
	{ EScriptGraphColor::Yellow, Gdiplus::Color(250, 232, 12)  },
	{ EScriptGraphColor::Orange, Gdiplus::Color(255, 100, 15)  },
	{ EScriptGraphColor::Purple, Gdiplus::Color(125, 55,  125) },
};

inline Gdiplus::Color GetColorValue(EScriptGraphColor color)
{
	for (uint32 colorIdx = 0; colorIdx < CRY_ARRAY_COUNT(g_colorValues); ++colorIdx)
	{
		if (g_colorValues[colorIdx].color == color)
		{
			return g_colorValues[colorIdx].value;
		}
	}
	return Gdiplus::Color();
}

static const struct
{
	SGUID          typeGUID;
	Gdiplus::Color color;
} g_portColors[] =
{
	{ GetTypeInfo<bool>().GetGUID(),          Gdiplus::Color(0,   108, 217) },
	{ GetTypeInfo<int32>().GetGUID(),         Gdiplus::Color(215, 55,  55)  },
	{ GetTypeInfo<uint32>().GetGUID(),        Gdiplus::Color(215, 55,  55)  },
	{ GetTypeInfo<float>().GetGUID(),         Gdiplus::Color(185, 185, 185) },
	{ GetTypeInfo<Vec3>().GetGUID(),          Gdiplus::Color(250, 232, 12)  },
	{ GetTypeInfo<SGUID>().GetGUID(),         Gdiplus::Color(38,  184, 33)  },
	{ GetTypeInfo<CSharedString>().GetGUID(), Gdiplus::Color(128, 100, 162) }
};

inline Gdiplus::Color GetPortColor(const SGUID& typeGUID)
{
	for (uint32 portColorIdx = 0; portColorIdx < CRY_ARRAY_COUNT(g_portColors); ++portColorIdx)
	{
		if (g_portColors[portColorIdx].typeGUID == typeGUID)
		{
			return g_portColors[portColorIdx].color;
		}
	}
	return Gdiplus::Color(28, 212, 22);
}

struct SNodeConnectionState
{
	inline SNodeConnectionState(const CScriptGraphViewNodePtr& _pNode)
		: pNode(_pNode)
		, bConnected(false)
	{}

	CScriptGraphViewNodePtr pNode;
	bool                    bConnected;
};

typedef std::vector<SNodeConnectionState> NodeConnectionStates; // #SchematycTODO : Replace with VectorMap for faster lookups?

inline void SetNodeConnectionStatesBackwardsRecursive(const IScriptGraph& scriptGraph, NodeConnectionStates& nodeConnectionStates, const SGUID& nodeGUID, const CGraphPortId& outputId)
{
	NodeConnectionStates::iterator itNodeConnectionState = std::find_if(nodeConnectionStates.begin(), nodeConnectionStates.end(), [&nodeGUID](const SNodeConnectionState& nodeConnectionState) -> bool { return nodeConnectionState.pNode->GetGUID() == nodeGUID; });
	if (itNodeConnectionState != nodeConnectionStates.end())
	{
		if (!itNodeConnectionState->bConnected)
		{
			const IScriptGraphNode& scriptGraphNode = itNodeConnectionState->pNode->GetScriptGraphNode();

			if (scriptGraphNode.GetOutputFlags(scriptGraphNode.FindOutputById(outputId)).Check(EScriptGraphPortFlags::Pull))
			{
				itNodeConnectionState->bConnected = true;

				for (uint32 inputIdx = 0, inputCount = scriptGraphNode.GetInputCount(); inputIdx < inputCount; ++inputIdx)
				{
					if (scriptGraphNode.GetInputFlags(inputIdx).Check(EScriptGraphPortFlags::Data))
					{
						auto visitScriptGraphLink = [&scriptGraph, &nodeConnectionStates](const IScriptGraphLink& scriptGraphLink) -> EVisitStatus
						{
							SetNodeConnectionStatesBackwardsRecursive(scriptGraph, nodeConnectionStates, scriptGraphLink.GetSrcNodeGUID(), scriptGraphLink.GetSrcOutputId());
							return EVisitStatus::Continue;
						};
						scriptGraph.VisitInputLinks(ScriptGraphLinkConstVisitor::FromLambda(visitScriptGraphLink), nodeGUID, scriptGraphNode.GetInputId(inputIdx));
					}
				}
			}
		}
	}
}

inline void SetNodeConnectionStatesForwardsRecursive(const IScriptGraph& scriptGraph, NodeConnectionStates& nodeConnectionStates, const SGUID& nodeGUID)
{
	NodeConnectionStates::iterator itNodeConnectionState = std::find_if(nodeConnectionStates.begin(), nodeConnectionStates.end(), [&nodeGUID](const SNodeConnectionState& nodeConnectionState) -> bool { return nodeConnectionState.pNode->GetGUID() == nodeGUID; });
	if (itNodeConnectionState != nodeConnectionStates.end())
	{
		if (!itNodeConnectionState->bConnected)
		{
			itNodeConnectionState->bConnected = true;

			const IScriptGraphNode& scriptGraphNode = itNodeConnectionState->pNode->GetScriptGraphNode();

			for (uint32 inputIdx = 0, inputCount = scriptGraphNode.GetInputCount(); inputIdx < inputCount; ++inputIdx)
			{
				if (scriptGraphNode.GetInputFlags(inputIdx).Check(EScriptGraphPortFlags::Data))
				{
					auto visitScriptGraphLink = [&scriptGraph, &nodeConnectionStates](const IScriptGraphLink& scriptGraphLink) -> EVisitStatus
					{
						SetNodeConnectionStatesBackwardsRecursive(scriptGraph, nodeConnectionStates, scriptGraphLink.GetSrcNodeGUID(), scriptGraphLink.GetSrcOutputId());
						return EVisitStatus::Continue;
					};
					scriptGraph.VisitInputLinks(ScriptGraphLinkConstVisitor::FromLambda(visitScriptGraphLink), nodeGUID, scriptGraphNode.GetInputId(inputIdx));
				}
			}

			for (uint32 outputIdx = 0, outputCount = scriptGraphNode.GetOutputCount(); outputIdx < outputCount; ++outputIdx)
			{
				if (scriptGraphNode.GetOutputFlags(outputIdx).CheckAny({ EScriptGraphPortFlags::Signal, EScriptGraphPortFlags::Flow }))
				{
					auto visitScriptGraphLink = [&scriptGraph, &nodeConnectionStates](const IScriptGraphLink& scriptGraphLink) -> EVisitStatus
					{
						SetNodeConnectionStatesForwardsRecursive(scriptGraph, nodeConnectionStates, scriptGraphLink.GetDstNodeGUID());
						return EVisitStatus::Continue;
					};
					scriptGraph.VisitOutputLinks(ScriptGraphLinkConstVisitor::FromLambda(visitScriptGraphLink), nodeGUID, scriptGraphNode.GetOutputId(outputIdx));
				}
			}
		}
	}
}
}

CScriptGraphViewNode::CScriptGraphViewNode(const SGraphViewGrid& grid, Gdiplus::PointF pos, IScriptGraph& scriptGraph, IScriptGraphNode& scriptGraphNode)
	: CGraphViewNode(grid, pos)
	, m_scriptGraph(scriptGraph)
	, m_scriptGraphNode(scriptGraphNode)
{}

const char* CScriptGraphViewNode::GetName() const
{
	return m_scriptGraphNode.GetName();
}

Gdiplus::Color CScriptGraphViewNode::GetHeaderColor() const
{
	ColorB color = m_scriptGraphNode.GetColor();
	return Gdiplus::Color(color.a, color.r, color.g, color.b);
}

bool CScriptGraphViewNode::IsRemoveable() const
{
	return m_scriptGraphNode.GetFlags().Check(EScriptGraphNodeFlags::Removable);
}

uint32 CScriptGraphViewNode::GetInputCount() const
{
	return m_scriptGraphNode.GetInputCount();
}

uint32 CScriptGraphViewNode::FindInputById(const CGraphPortId& id) const
{
	return m_scriptGraphNode.FindInputById(id);
}

CGraphPortId CScriptGraphViewNode::GetInputId(uint32 inputIdx) const
{
	return m_scriptGraphNode.GetInputId(inputIdx);
}

const char* CScriptGraphViewNode::GetInputName(uint32 inputIdx) const
{
	return m_scriptGraphNode.GetInputName(inputIdx);
}

ScriptGraphPortFlags CScriptGraphViewNode::GetInputFlags(uint32 inputIdx) const
{
	return m_scriptGraphNode.GetInputFlags(inputIdx);
}

Gdiplus::Color CScriptGraphViewNode::GetInputColor(uint32 inputIdx) const
{
	return GetPortColor(m_scriptGraphNode.GetInputTypeGUID(inputIdx));
}

uint32 CScriptGraphViewNode::GetOutputCount() const
{
	return m_scriptGraphNode.GetOutputCount();
}

uint32 CScriptGraphViewNode::FindOutputById(const CGraphPortId& id) const
{
	return m_scriptGraphNode.FindOutputById(id);
}

CGraphPortId CScriptGraphViewNode::GetOutputId(uint32 outputIdx) const
{
	return m_scriptGraphNode.GetOutputId(outputIdx);
}

const char* CScriptGraphViewNode::GetOutputName(uint32 outputIdx) const
{
	return m_scriptGraphNode.GetOutputName(outputIdx);
}

ScriptGraphPortFlags CScriptGraphViewNode::GetOutputFlags(uint32 outputIdx) const
{
	return m_scriptGraphNode.GetOutputFlags(outputIdx);
}

Gdiplus::Color CScriptGraphViewNode::GetOutputColor(uint32 outputIdx) const
{
	return GetPortColor(m_scriptGraphNode.GetOutputTypeGUID(outputIdx));
}

void CScriptGraphViewNode::CopyToXml(IString& output) const
{
	m_scriptGraphNode.CopyToXml(output);
}

IScriptGraphNode& CScriptGraphViewNode::GetScriptGraphNode()
{
	return m_scriptGraphNode;
}

const IScriptGraphNode& CScriptGraphViewNode::GetScriptGraphNode() const
{
	return m_scriptGraphNode;
}

SGUID CScriptGraphViewNode::GetGUID() const
{
	return m_scriptGraphNode.GetGUID();
}

void CScriptGraphViewNode::Serialize(Serialization::IArchive& archive)
{
	m_scriptGraphNode.Serialize(archive);
}

void CScriptGraphViewNode::OnMove(Gdiplus::RectF paintRect)
{
	m_scriptGraphNode.SetPos(Vec2(paintRect.X, paintRect.Y));
}

SScriptGraphViewSelection::SScriptGraphViewSelection(const CScriptGraphViewNodePtr& _pNode, SGraphViewLink* _pLink)
	: pNode(_pNode)
	, pLink(_pLink)
{}

BEGIN_MESSAGE_MAP(CScriptGraphView, CGraphView)
END_MESSAGE_MAP()

CScriptGraphView::CScriptGraphView()
	: CGraphView(SGraphViewGrid(Gdiplus::PointF(10.0f, 10.0f), Gdiplus::RectF(-5000.0f, -5000.0f, 10000.0f, 10000.0f)))
	, m_pScriptGraph(nullptr)
	, m_bRemovingLinks(false)
{
	GetSchematycFramework().GetScriptRegistry().GetChangeSignalSlots().Connect(Schematyc::Delegate::Make(*this, &CScriptGraphView::OnScriptRegistryChange), m_connectionScope);
	CGraphView::Enable(false);
}

void CScriptGraphView::Refresh()
{
	if (m_pScriptGraph)
	{
		m_pScriptGraph->GetElement().ProcessEvent(SScriptEvent(EScriptEventId::EditorSelect));
	}
	CGraphView::Refresh();
}

void CScriptGraphView::Load(IScriptGraph* pScriptGraph)
{
	if (m_pScriptGraph)
	{
		m_pScriptGraph->GetLinkRemovedSignalSlots().Disconnect(m_connectionScope);
		CGraphView::Clear();
	}
	m_pScriptGraph = pScriptGraph;
	if (m_pScriptGraph)
	{
		CGraphView::Enable(true);
		const Vec2 pos = m_pScriptGraph->GetPos();
		CGraphView::SetScrollOffset(Gdiplus::PointF(pos.x, pos.y));
		switch (m_pScriptGraph->GetType())
		{
		case EScriptGraphType::Logic:
		case EScriptGraphType::Transition:
			{
				static_cast<IScriptGraph*>(m_pScriptGraph)->ProcessEvent(SScriptEvent(EScriptEventId::EditorSelect));   // #SchematycTODO : Once all graphs are implemented as script extensions we can remove this cast!
				break;
			}
		default:
			{
				m_pScriptGraph->GetElement().ProcessEvent(SScriptEvent(EScriptEventId::EditorSelect));
				break;
			}
		}
		m_pScriptGraph->VisitNodes(Schematyc::Delegate::Make(*this, &CScriptGraphView::VisitGraphNode));
		for (uint32 iLink = 0, linkCount = m_pScriptGraph->GetLinkCount(); iLink < linkCount; )
		{
			if (AddLink(*m_pScriptGraph->GetLink(iLink)))
			{
				++iLink;
			}
			else
			{
				// #SchematycTODO : May make more sense to strip out broken links when loading the script.
				SCHEMATYC_EDITOR_WARNING("Removing broken link from graph : %s", pScriptGraph->GetElement().GetName());
				m_bRemovingLinks = true;
				m_pScriptGraph->RemoveLink(iLink);
				m_bRemovingLinks = false;
				--linkCount;
			}
		}
		m_pScriptGraph->GetLinkRemovedSignalSlots().Connect(Schematyc::Delegate::Make(*this, &CScriptGraphView::OnScriptGraphLinkRemoved), m_connectionScope);
	}
	else
	{
		CGraphView::Enable(false);
	}
	ValidateNodes();
	EnableConnectedNodes();
	__super::Invalidate(TRUE);
}

IScriptGraph* CScriptGraphView::GetScriptGraph() const
{
	return m_pScriptGraph;
}

void CScriptGraphView::SelectAndFocusNode(const SGUID& nodeGuid)
{
	if (!GUID::IsEmpty(nodeGuid))
	{
		CGraphViewNodePtr pNode = GetNode(nodeGuid);
		if (pNode)
		{
			SelectNode(pNode);
			Gdiplus::RectF rect = pNode->GetPaintRect();
			Gdiplus::PointF pos(rect.X + (rect.Width / 2), rect.Y + (rect.Height / 2));
			CenterPosition(pos);
		}
	}
}

ScriptGraphViewSelectionSignal::Slots& CScriptGraphView::GetSelectionSignalSlots()
{
	return m_signals.selection.GetSlots();
}

ScriptGraphViewModificationSignal::Slots& CScriptGraphView::GetModificationSignalSlots()
{
	return m_signals.modification.GetSlots();
}

void CScriptGraphView::OnMove(Gdiplus::PointF scrollOffset)
{
	if (m_pScriptGraph)
	{
		m_pScriptGraph->SetPos(Vec2(scrollOffset.X, scrollOffset.Y));
	}
}

void CScriptGraphView::OnSelection(const CGraphViewNodePtr& pSelectedNode, SGraphViewLink* pSelectedLink)
{
	m_signals.selection.Send(SScriptGraphViewSelection(std::static_pointer_cast<CScriptGraphViewNode>(pSelectedNode), pSelectedLink));
}

void CScriptGraphView::OnLinkModify(const SGraphViewLink& link) {}

void CScriptGraphView::OnNodeRemoved(const CGraphViewNode& node)
{
	SCHEMATYC_EDITOR_ASSERT(m_pScriptGraph);
	if (m_pScriptGraph)
	{
		m_pScriptGraph->RemoveNode(static_cast<const CScriptGraphViewNode&>(node).GetGUID());
		InvalidateScriptGraph();
	}
}

bool CScriptGraphView::CanCreateLink(const CGraphViewNode& srcNode, const CGraphPortId& srcOutputId, const CGraphViewNode& dstNode, const CGraphPortId& dstInputId) const
{
	SCHEMATYC_EDITOR_ASSERT(m_pScriptGraph);
	if (m_pScriptGraph)
	{
		const CScriptGraphViewNode& srcNodeImpl = static_cast<const CScriptGraphViewNode&>(srcNode);
		const CScriptGraphViewNode& dstNodeImpl = static_cast<const CScriptGraphViewNode&>(dstNode);
		return m_pScriptGraph->CanAddLink(srcNodeImpl.GetGUID(), srcOutputId, dstNodeImpl.GetGUID(), dstInputId);
	}
	return false;
}

void CScriptGraphView::OnLinkCreated(const SGraphViewLink& link)
{
	SCHEMATYC_EDITOR_ASSERT(m_pScriptGraph);
	if (m_pScriptGraph)
	{
		CScriptGraphViewNodePtr pSrcNode = std::static_pointer_cast<CScriptGraphViewNode>(link.pSrcNode.lock());
		CScriptGraphViewNodePtr pDstNode = std::static_pointer_cast<CScriptGraphViewNode>(link.pDstNode.lock());
		SCHEMATYC_EDITOR_ASSERT(pSrcNode && pDstNode);
		if (pSrcNode && pDstNode)
		{
			m_pScriptGraph->AddLink(pSrcNode->GetGUID(), link.srcOutputId, pDstNode->GetGUID(), link.dstInputId);
			InvalidateScriptGraph();
		}
	}
}

void CScriptGraphView::OnLinkRemoved(const SGraphViewLink& link)
{
	if (!m_bRemovingLinks)
	{
		SCHEMATYC_EDITOR_ASSERT(m_pScriptGraph);
		if (m_pScriptGraph)
		{
			CScriptGraphViewNodePtr pSrcNode = std::static_pointer_cast<CScriptGraphViewNode>(link.pSrcNode.lock());
			CScriptGraphViewNodePtr pDstNode = std::static_pointer_cast<CScriptGraphViewNode>(link.pDstNode.lock());
			SCHEMATYC_EDITOR_ASSERT(pSrcNode && pDstNode);
			if (pSrcNode && pDstNode)
			{
				const uint32 linkIdx = m_pScriptGraph->FindLink(pSrcNode->GetGUID(), link.srcOutputId, pDstNode->GetGUID(), link.dstInputId);
				if (linkIdx != InvalidIdx)
				{
					m_bRemovingLinks = true;
					m_pScriptGraph->RemoveLink(linkIdx);
					m_bRemovingLinks = false;
					InvalidateScriptGraph();
				}
			}
		}
	}
}

DROPEFFECT CScriptGraphView::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	if (m_pScriptGraph)
	{
		PluginUtils::SDragAndDropData dragAndDropData;
		if (PluginUtils::GetDragAndDropData(pDataObject, dragAndDropData))
		{
			/*switch(dragAndDropData.icon)
			   {
			   case BrowserIcon::BRANCH:
			   {
			    if(CanAddNode(EScriptGraphNodeType::Branch))
			    {
			      return DROPEFFECT_MOVE;
			    }
			    break;
			   }
			   case BrowserIcon::FOR_LOOP:
			   {
			    if(CanAddNode(EScriptGraphNodeType::ForLoop))
			    {
			      return DROPEFFECT_MOVE;
			    }
			    break;
			   }
			   case BrowserIcon::RETURN:
			   {
			    if(CanAddNode(EScriptGraphNodeType::Return))
			    {
			      return DROPEFFECT_MOVE;
			    }
			    break;
			   }
			   default:
			   {
			    if(CanAddNode(dragAndDropData.guid))
			    {
			      return DROPEFFECT_MOVE;
			    }
			    break;
			   }
			   }*/
		}
	}
	return DROPEFFECT_NONE;
}

BOOL CScriptGraphView::OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	if (m_pScriptGraph)
	{
		PluginUtils::SDragAndDropData dragAndDropData;
		if (PluginUtils::GetDragAndDropData(pDataObject, dragAndDropData))
		{
			const float dragAndDropOffset = 10.0f;
			const Gdiplus::PointF graphPos = CGraphView::ClientToGraph(Gdiplus::PointF(static_cast<float>(point.x), static_cast<float>(point.y)));
			/*switch(dragAndDropData.icon)
			   {
			   case BrowserIcon::BRANCH:
			   {
			    AddNode(EScriptGraphNodeType::Branch, SGUID(), SGUID(), Gdiplus::PointF(graphPos.X - dragAndDropOffset, graphPos.Y - dragAndDropOffset));
			    InvalidateDoc();
			    return true;
			   }
			   case BrowserIcon::FOR_LOOP:
			   {
			    AddNode(EScriptGraphNodeType::ForLoop, SGUID(), SGUID(), Gdiplus::PointF(graphPos.X - dragAndDropOffset, graphPos.Y - dragAndDropOffset));
			    InvalidateDoc();
			    return true;
			   }
			   case BrowserIcon::RETURN:
			   {
			    AddNode(EScriptGraphNodeType::Return, SGUID(), SGUID(), Gdiplus::PointF(graphPos.X - dragAndDropOffset, graphPos.Y - dragAndDropOffset));
			    InvalidateDoc();
			    return true;
			   }
			   default:
			   {
			    switch(m_pScriptGraph->GetType())
			    {
			    case EScriptGraphType::Property:
			    case EScriptGraphType::Logic:
			    case EScriptGraphType::Transition:
			      {
			        break;
			      }
			    default:
			      {
			        UInt32Vector availableNodes;
			        m_pScriptGraph->RefreshAvailableNodes(SElementId());
			        for(uint32 iAvalailableNode = 0, availableNodeCount = m_pScriptGraph->GetAvailableNodeCount(); iAvalailableNode < availableNodeCount; ++ iAvalailableNode)
			        {
			          if(m_pScriptGraph->GetAvailableNodeRefGUID(iAvalailableNode) == dragAndDropData.guid)
			          {
			            availableNodes.push_back(iAvalailableNode);
			          }
			        }
			        if(!availableNodes.empty())
			        {
			          int	nodeSelection = availableNodes.front() + 1;
			          if(availableNodes.size() > 1)
			          {
			            CMenu popupMenu;
			            popupMenu.CreatePopupMenu();
			            for(UInt32Vector::const_iterator iAvailableNode = availableNodes.begin(), iEndAvailableNode = availableNodes.end(); iAvailableNode != iEndAvailableNode; ++ iAvailableNode)
			            {
			              popupMenu.AppendMenu(MF_STRING, *iAvailableNode + 1, m_pScriptGraph->GetAvailableNodeName(*iAvailableNode));
			            }
			            CPoint cursorPos;
			            GetCursorPos(&cursorPos);
			            nodeSelection = popupMenu.TrackPopupMenuEx(TPM_RETURNCMD, cursorPos.x, cursorPos.y, this, NULL);
			          }
			          if(nodeSelection > 0)
			          {
			            AddNode(m_pScriptGraph->GetAvailableNodeType(nodeSelection - 1), SGUID(), dragAndDropData.guid, Gdiplus::PointF(graphPos.X - dragAndDropOffset, graphPos.Y - dragAndDropOffset));
			            InvalidateDoc();
			            return true;
			          }
			        }
			        break;
			      }
			    }
			    break;
			   }
			   }*/
		}
	}
	return false;
}

void CScriptGraphView::GetPopupMenuItems(CMenu& popupMenu, const CGraphViewNodePtr& pNode, uint32 nodeInputIdx, uint32 nodeOutputIdx, CPoint point)
{
	CGraphView::GetPopupMenuItems(popupMenu, pNode, nodeInputIdx, nodeOutputIdx, point);
}

void CScriptGraphView::OnPopupMenuResult(BOOL popupMenuItem, const CGraphViewNodePtr& pNode, uint32 nodeInputIdx, uint32 nodeOutputIdx, CPoint point)
{
	CGraphView::OnPopupMenuResult(popupMenuItem, pNode, nodeInputIdx, nodeOutputIdx, point);
}

const IQuickSearchOptions* CScriptGraphView::GetQuickSearchOptions(CPoint point, const CGraphViewNodePtr& pNode, uint32 nodeOutputIdx)
{
	if (m_pScriptGraph)
	{
		m_quickSearchOptions.Refresh(*m_pScriptGraph);
		return &m_quickSearchOptions;
	}
	return NULL;
}

void CScriptGraphView::OnQuickSearchResult(CPoint point, const CGraphViewNodePtr& pNode, uint32 nodeOutputIdx, uint32 optionIdx)
{
	SCHEMATYC_EDITOR_ASSERT((optionIdx < m_quickSearchOptions.GetCount()));
	if (m_pScriptGraph && (optionIdx < m_quickSearchOptions.GetCount()))
	{
		CGraphView::ScreenToClient(&point);
		const Gdiplus::PointF nodePos = CGraphView::ClientToGraph(point.x, point.y);
		CScriptGraphViewNodePtr pNewNode;

		IScriptGraphNodePtr pScriptGraphNode = m_quickSearchOptions.ExecuteCommand(optionIdx, Vec2(nodePos.X, nodePos.Y));   // #SchematycTODO : Once all graphs are implemented as script extensions we can remove this cast!
		if (pScriptGraphNode)
		{
			static_cast<IScriptGraph*>(m_pScriptGraph)->AddNode(pScriptGraphNode);

			pScriptGraphNode->ProcessEvent(SScriptEvent(EScriptEventId::EditorAdd));

			pNewNode = AddNode(*pScriptGraphNode);
			if (pNode && (nodeOutputIdx != InvalidIdx))
			{
				for (uint32 nodeInputIdx = 0, nodeInputCount = pNewNode->GetInputCount(); nodeInputIdx < nodeInputCount; ++nodeInputIdx)
				{
					if (CGraphView::CanCreateLink(*pNode, pNode->GetOutputId(nodeOutputIdx), *pNewNode, pNewNode->GetInputId(nodeInputIdx)))
					{
						CGraphView::CreateLink(pNode, pNode->GetOutputId(nodeOutputIdx), pNewNode, pNewNode->GetInputId(nodeInputIdx));
						break;
					}
				}
			}

			InvalidateScriptGraph();
		}
	}
}

const char* CScriptGraphView::CQuickSearchOptions::GetHeader() const
{
	return nullptr;
}

const char* CScriptGraphView::CQuickSearchOptions::GetDelimiter() const
{
	return "::";
}

uint32 CScriptGraphView::CQuickSearchOptions::GetCount() const
{
	return m_options.size();
}

const char* CScriptGraphView::CQuickSearchOptions::GetLabel(uint32 optionIdx) const
{
	return optionIdx < m_options.size() ? m_options[optionIdx].label.c_str() : "";
}

const char* CScriptGraphView::CQuickSearchOptions::GetDescription(uint32 optionIdx) const
{
	return optionIdx < m_options.size() ? m_options[optionIdx].description.c_str() : "";
}

const char* CScriptGraphView::CQuickSearchOptions::GetWikiLink(uint32 optionIdx) const
{
	return optionIdx < m_options.size() ? m_options[optionIdx].wikiLink.c_str() : "";
}

bool CScriptGraphView::CQuickSearchOptions::AddOption(const char* szLabel, const char* szDescription, const char* szWikiLink, const IScriptGraphNodeCreationMenuCommandPtr& pCommand)
{
	// #SchematycTODO : Check for duplicate options?
	m_options.push_back(SOption(szLabel, szDescription, szWikiLink, pCommand));
	return true;
}

void CScriptGraphView::CQuickSearchOptions::Refresh(IScriptGraph& scriptGraph)
{
	m_options.clear();
	static_cast<IScriptGraph&>(scriptGraph).PopulateNodeCreationMenu(*this);   // #SchematycTODO : Once all graphs are implemented as script extensions we can remove this cast!
}

SGUID CScriptGraphView::CQuickSearchOptions::GetNodeContextGUID(uint32 iOption) const
{
	return iOption < m_options.size() ? m_options[iOption].nodeContextGUID : SGUID();
}

SGUID CScriptGraphView::CQuickSearchOptions::GetNodeRefGUID(uint32 iOption) const
{
	return iOption < m_options.size() ? m_options[iOption].nodeRefGUID : SGUID();
}

IScriptGraphNodePtr CScriptGraphView::CQuickSearchOptions::ExecuteCommand(uint32 optionIdx, const Vec2& pos)
{
	if (optionIdx < m_options.size())
	{
		IScriptGraphNodeCreationMenuCommandPtr& pCommand = m_options[optionIdx].pCommand;
		if (pCommand)
		{
			return pCommand->Execute(pos);
		}
	}
	return IScriptGraphNodePtr();
}

CScriptGraphView::CQuickSearchOptions::SOption::SOption(const char* _szLabel, const char* _szDescription, const char* _szWikiLink, const IScriptGraphNodeCreationMenuCommandPtr& _pCommand)
	: label(_szLabel)
	, description(_szDescription)
	, wikiLink(_szWikiLink)
	, pCommand(_pCommand)
{}

EVisitStatus CScriptGraphView::VisitGraphNode(IScriptGraphNode& graphNode)
{
	AddNode(graphNode);
	return EVisitStatus::Continue;
}

void CScriptGraphView::OnScriptRegistryChange(const SScriptRegistryChange& change)
{
	if (m_pScriptGraph && (&change.element == &m_pScriptGraph->GetElement()))
	{
		switch (change.type)
		{
		case EScriptRegistryChangeType::ElementModified:
		case EScriptRegistryChangeType::ElementDependencyModified:
		case EScriptRegistryChangeType::ElementDependencyRemoved:
			{
				// #SchematycTODO : Keep an eye on efficiency of this, we probably don't need to validate all nodes after every change.
				//                : If it does become an issue this can probably be time sliced.
				ValidateNodes();
				__super::Invalidate(TRUE);
				break;
			}
		}
	}
}

CScriptGraphViewNodePtr CScriptGraphView::AddNode(IScriptGraphNode& scriptGraphNode)
{
	SCHEMATYC_EDITOR_ASSERT(m_pScriptGraph);
	if (m_pScriptGraph)
	{
		const Vec2 pos = scriptGraphNode.GetPos();
		CScriptGraphViewNodePtr pNode(new CScriptGraphViewNode(CGraphView::GetGrid(), Gdiplus::PointF(pos.x, pos.y), *m_pScriptGraph, scriptGraphNode));
		CGraphView::AddNode(pNode, false, false);
		return pNode;
	}
	return CScriptGraphViewNodePtr();
}

CScriptGraphViewNodePtr CScriptGraphView::GetNode(const SGUID& guid) const
{
	const TGraphViewNodePtrVector& nodes = GetNodes();
	for (TGraphViewNodePtrVector::const_iterator iNode = nodes.begin(), iEndNode = nodes.end(); iNode != iEndNode; ++iNode)
	{
		const CScriptGraphViewNodePtr& pNode = std::static_pointer_cast<CScriptGraphViewNode>(*iNode);
		if (pNode->GetGUID() == guid)
		{
			return pNode;
		}
	}
	return CScriptGraphViewNodePtr();
}

bool CScriptGraphView::AddLink(const IScriptGraphLink& scriptGraphLink)
{
	CScriptGraphViewNodePtr pSrcNode = std::static_pointer_cast<CScriptGraphViewNode>(GetNode(scriptGraphLink.GetSrcNodeGUID()));
	CScriptGraphViewNodePtr pDstNode = std::static_pointer_cast<CScriptGraphViewNode>(GetNode(scriptGraphLink.GetDstNodeGUID()));
	if (pSrcNode && pDstNode)
	{
		CGraphView::AddLink(pSrcNode, scriptGraphLink.GetSrcOutputId(), pDstNode, scriptGraphLink.GetDstInputId());
		return true;
	}
	return false;
}

void CScriptGraphView::InvalidateScriptGraph()
{
	if (m_pScriptGraph)
	{
		GetSchematycFramework().GetScriptRegistry().ElementModified(m_pScriptGraph->GetElement()); // #SchematycTODO : Call from script element?
		m_signals.modification.Send();                                                             // #SchematycTODO : Do we really need to send this now?
		EnableConnectedNodes();
	}
}

void CScriptGraphView::ValidateNodes()
{
	for (const CGraphViewNodePtr& pNode : CGraphView::GetNodes())
	{
		IValidatorArchivePtr pArchive = GetSchematycFramework().CreateValidatorArchive(SValidatorArchiveParams());
		ISerializationContextPtr pSerializationContext = GetSchematycFramework().CreateSerializationContext(SSerializationContextParams(*pArchive, ESerializationPass::Validate));
		std::static_pointer_cast<CScriptGraphViewNode>(pNode)->Serialize(*pArchive);

		GraphViewNodeStatusFlags graphViewNodeStatusFlags = pNode->GetStatusFlags();
		graphViewNodeStatusFlags.Remove({ EGraphViewNodeStatusFlags::ContainsWarnings, EGraphViewNodeStatusFlags::ContainsErrors });
		if (pArchive->GetWarningCount())
		{
			graphViewNodeStatusFlags.Add(EGraphViewNodeStatusFlags::ContainsWarnings);
		}
		if (pArchive->GetErrorCount())
		{
			graphViewNodeStatusFlags.Add(EGraphViewNodeStatusFlags::ContainsErrors);
		}
		pNode->SetStatusFlags(graphViewNodeStatusFlags);
	}
}

void CScriptGraphView::EnableConnectedNodes()
{
	if (m_pScriptGraph)
	{
		const TGraphViewNodePtrVector& nodes = CGraphView::GetNodes();
		const uint32 nodeCount = nodes.size();

		NodeConnectionStates nodeConnectionStates;
		nodeConnectionStates.reserve(nodeCount);

		for (const CGraphViewNodePtr& pNode : nodes)
		{
			nodeConnectionStates.push_back(std::static_pointer_cast<CScriptGraphViewNode>(pNode));
		}

		for (uint32 nodeIdx = 0; nodeIdx < nodeCount; ++nodeIdx)
		{
			SNodeConnectionState& nodeConnectionState = nodeConnectionStates[nodeIdx];
			const IScriptGraphNode& scriptGraphNode = nodeConnectionState.pNode->GetScriptGraphNode();
			for (uint32 outputIdx = 0, outputCount = scriptGraphNode.GetOutputCount(); outputIdx < outputCount; ++outputIdx)
			{
				if (scriptGraphNode.GetOutputFlags(outputIdx).CheckAny({ EScriptGraphPortFlags::Signal, EScriptGraphPortFlags::Begin }))
				{
					nodeConnectionState.bConnected = true;

					auto visitScriptGraphLink = [this, &nodeConnectionStates](const IScriptGraphLink& scriptGraphLink) -> EVisitStatus
					{
						SetNodeConnectionStatesForwardsRecursive(*m_pScriptGraph, nodeConnectionStates, scriptGraphLink.GetDstNodeGUID());
						return EVisitStatus::Continue;
					};
					m_pScriptGraph->VisitOutputLinks(ScriptGraphLinkConstVisitor::FromLambda(visitScriptGraphLink), scriptGraphNode.GetGUID(), scriptGraphNode.GetOutputId(outputIdx));
				}
			}
		}

		for (const SNodeConnectionState& nodeConnectionState : nodeConnectionStates)
		{
			nodeConnectionState.pNode->Enable(nodeConnectionState.bConnected);
		}
	}
}

void CScriptGraphView::OnScriptGraphLinkRemoved(const IScriptGraphLink& link)
{
	if (!m_bRemovingLinks)
	{
		const uint32 linkIdx = CGraphView::FindLink(GetNode(link.GetSrcNodeGUID()), link.GetSrcOutputId(), GetNode(link.GetDstNodeGUID()), link.GetDstInputId());
		if (linkIdx != InvalidIdx)
		{
			m_bRemovingLinks = true;
			CGraphView::RemoveLink(linkIdx);
			m_bRemovingLinks = false;
		}
	}
}

CGraphSettingsWidget::CGraphSettingsWidget(QWidget* pParent)
	: QWidget(pParent)
	, m_pGraphView(nullptr)
{
	m_pLayout = new QBoxLayout(QBoxLayout::TopToBottom);
	m_pPropertyTree = new QPropertyTree(this);
}

void CGraphSettingsWidget::SetGraphView(CGraphView* pGraphView)
{
	m_pGraphView = pGraphView;
}

void CGraphSettingsWidget::InitLayout()
{
	QWidget::setLayout(m_pLayout);
	m_pLayout->setSpacing(1);
	m_pLayout->setMargin(0);
	m_pLayout->addWidget(m_pPropertyTree, 1);

	m_pPropertyTree->setSizeHint(QSize(250, 400));
	m_pPropertyTree->setExpandLevels(1);
	m_pPropertyTree->setSliderUpdateDelay(5);
	m_pPropertyTree->setValueColumnWidth(0.6f);
	m_pPropertyTree->attach(Serialization::SStruct(*this));
}

void CGraphSettingsWidget::Serialize(Serialization::IArchive& archive)
{
	if (m_pGraphView)
	{
		archive(m_pGraphView->GetSettings(), "general", "General");
		archive(m_pGraphView->GetPainter()->GetSettings(), "painter", "Painter");
	}
}

CScriptGraphWidget::CScriptGraphWidget(QWidget* pParent, CWnd* pParentWnd)
	: QWidget(pParent)
{
	m_pMainLayout = new QBoxLayout(QBoxLayout::TopToBottom);
	m_pSplitter = new QSplitter(Qt::Horizontal, this);
	m_pSettings = new CGraphSettingsWidget(this);
	m_pGraphViewHost = new QWinHost(this);
	m_pGraphView = new CScriptGraphView();
	m_pControlLayout = new QBoxLayout(QBoxLayout::LeftToRight);
	m_pShowHideSettingsButton = new QPushButton(">> Show Settings", this);

	m_pGraphView->Create(WS_CHILD, CRect(0, 0, 0, 0), pParentWnd, 0);
	m_pGraphViewHost->setWindow(m_pGraphView->GetSafeHwnd());

	m_pSettings->SetGraphView(m_pGraphView);
	m_pSettings->setVisible(false);

	connect(m_pShowHideSettingsButton, SIGNAL(clicked()), this, SLOT(OnShowHideSettingsButtonClicked()));
}

void CScriptGraphWidget::InitLayout()
{
	QWidget::setLayout(m_pMainLayout);
	m_pMainLayout->setSpacing(1);
	m_pMainLayout->setMargin(0);
	m_pMainLayout->addWidget(m_pSplitter, 1);
	m_pMainLayout->addLayout(m_pControlLayout, 0);

	m_pSplitter->setStretchFactor(0, 1);
	m_pSplitter->setStretchFactor(1, 0);
	m_pSplitter->addWidget(m_pSettings);
	m_pSplitter->addWidget(m_pGraphViewHost);

	QList<int> splitterSizes;
	splitterSizes.push_back(60);
	splitterSizes.push_back(200);
	m_pSplitter->setSizes(splitterSizes);

	m_pSettings->InitLayout();

	m_pControlLayout->setSpacing(2);
	m_pControlLayout->setMargin(4);
	m_pControlLayout->addWidget(m_pShowHideSettingsButton, 1);
}

void CScriptGraphWidget::LoadSettings(const XmlNodeRef& xml)
{
	Serialization::LoadXmlNode(*m_pSettings, xml);
}

XmlNodeRef CScriptGraphWidget::SaveSettings(const char* szName)
{
	return Serialization::SaveXmlNode(*m_pSettings, szName);
}

void CScriptGraphWidget::Refresh()
{
	m_pGraphView->Refresh();
}

void CScriptGraphWidget::LoadGraph(IScriptGraph* pScriptGraph)
{
	m_pGraphView->Load(pScriptGraph);
}

IScriptGraph* CScriptGraphWidget::GetScriptGraph() const
{
	return m_pGraphView->GetScriptGraph();
}

void CScriptGraphWidget::SelectAndFocusNode(const SGUID& nodeGuid)
{
	m_pGraphView->SelectAndFocusNode(nodeGuid);
}

ScriptGraphViewSelectionSignal::Slots& CScriptGraphWidget::GetSelectionSignalSlots()
{
	return m_pGraphView->GetSelectionSignalSlots();
}

ScriptGraphViewModificationSignal::Slots& CScriptGraphWidget::GetModificationSignalSlots()
{
	return m_pGraphView->GetModificationSignalSlots();
}

void CScriptGraphWidget::OnShowHideSettingsButtonClicked()
{
	const bool bShowSettings = !m_pSettings->isVisible();
	m_pShowHideSettingsButton->setText(bShowSettings ? "<< Hide Settings" : ">> Show Settings");
	m_pSettings->setVisible(bShowSettings);
}
} // Schematyc
