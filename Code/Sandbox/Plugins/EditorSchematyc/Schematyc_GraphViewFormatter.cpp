// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// Known issues: Trailing nodes need to be moved away by width of widest node in column, not just the width of current node.
//               It's possible to create infinite loops.

#include "StdAfx.h"
#include "Schematyc_GraphViewFormatter.h"

namespace Schematyc
{
	CGraphViewFormatter::CGraphViewFormatter(const CGraphView& graphView, const SGraphViewFormatSettings& settings)
		: m_graphView(graphView)
		, m_settings(settings)
	{}

	void CGraphViewFormatter::FormatNodes(TGraphViewNodePtrVector& nodes) const
	{
		TGraphViewNodePtrVector unformattedNodes(nodes);
		TGraphViewNodePtrVector formattedNodes;
		for(CGraphViewNodePtr& pNode : nodes)
		{
			if(IsBeginNode(pNode))
			{
				unformattedNodes.erase(std::find(unformattedNodes.begin(), unformattedNodes.end(), pNode));
				formattedNodes.push_back(pNode);
				FormatNodeForwardsRecursive(unformattedNodes, formattedNodes, pNode);
			}
		}
	}

	bool CGraphViewFormatter::IsBeginNode(const CGraphViewNodeConstPtr& pNode) const
	{
		for(uint32 inputIdx = 0, inputCount = pNode->GetInputCount(); inputIdx < inputCount; ++ inputIdx)
		{
			if (pNode->GetInputFlags(inputIdx).CheckAny({ EScriptGraphPortFlags::Signal, EScriptGraphPortFlags::Flow }))
			{
				const CGraphPortId inputId = pNode->GetInputId(inputIdx);
				for(const SGraphViewLink& link : m_graphView.GetLinks())
				{
					if((link.pDstNode.lock() == pNode) && (link.dstInputId == inputId))
					{
						return false;
					}
				}
			}
		}
		for(uint32 outputIdx = 0, outputCount = pNode->GetOutputCount(); outputIdx < outputCount; ++ outputIdx)
		{
			if(pNode->GetOutputFlags(outputIdx).Check(EScriptGraphPortFlags::Begin))
			{
				const CGraphPortId outputId = pNode->GetOutputId(outputIdx);
				for(const SGraphViewLink& link : m_graphView.GetLinks())
				{
					if((link.pSrcNode.lock() == pNode) && (link.srcOutputId == outputId))
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	void CGraphViewFormatter::FormatNodeForwardsRecursive(TGraphViewNodePtrVector& unformattedNodes, TGraphViewNodePtrVector& formattedNodes, const CGraphViewNodePtr& pNode) const
	{
		FormatNodeBackwardsRecursive(unformattedNodes, formattedNodes, pNode);
		const Gdiplus::RectF paintRect(pNode->GetPaintRect());
		Gdiplus::PointF      nextNodePos(paintRect.GetRight() + m_settings.horzSpacing, paintRect.GetTop());
		for(uint32 outputIdx = 0, outputCount = pNode->GetOutputCount(); outputIdx < outputCount; ++ outputIdx)
		{
			if (pNode->GetOutputFlags(outputIdx).CheckAny({ EScriptGraphPortFlags::Signal, EScriptGraphPortFlags::Flow }))
			{
				const CGraphPortId outputId = pNode->GetOutputId(outputIdx);
				for(const SGraphViewLink& link : m_graphView.GetLinks())
				{
					if((link.pSrcNode.lock() == pNode) && (link.srcOutputId == outputId))
					{
						CGraphViewNodePtr pNextNode = link.pDstNode.lock();
						if(pNextNode)
						{
							FormatNode(unformattedNodes, formattedNodes, pNextNode, nextNodePos);
							nextNodePos.Y = pNextNode->GetPaintRect().GetBottom() + m_settings.vertSpacing;
							FormatNodeForwardsRecursive(unformattedNodes, formattedNodes, pNextNode);
						}
					}
				}
			}
		}
	}

	void CGraphViewFormatter::FormatNodeBackwardsRecursive(TGraphViewNodePtrVector& unformattedNodes, TGraphViewNodePtrVector& formattedNodes, const CGraphViewNodePtr& pNode) const
	{
		const Gdiplus::RectF paintRect(pNode->GetPaintRect());
		Gdiplus::PointF      prevNodePos(paintRect.GetLeft() - m_settings.horzSpacing, paintRect.GetTop());
		for(uint32 inputIdx = 0, inputCount = pNode->GetInputCount(); inputIdx < inputCount; ++ inputIdx)
		{
			if(pNode->GetInputFlags(inputIdx).Check(EScriptGraphPortFlags::Data))
			{
				const CGraphPortId inputId = pNode->GetInputId(inputIdx);
				for(const SGraphViewLink& link : m_graphView.GetLinks())
				{
					if((link.pDstNode.lock() == pNode) && (link.dstInputId == inputId))
					{
						CGraphViewNodePtr pPrevNode = link.pSrcNode.lock();
						if(pPrevNode)
						{
							if(pPrevNode->GetOutputFlags(pPrevNode->FindOutputById(link.srcOutputId)).Check(EScriptGraphPortFlags::Pull))
							{
								FormatNode(unformattedNodes, formattedNodes, pPrevNode, Gdiplus::PointF(prevNodePos.X - pPrevNode->GetPaintRect().Width, prevNodePos.Y));
								prevNodePos.Y = pPrevNode->GetPaintRect().GetBottom() + m_settings.vertSpacing;
								FormatNodeBackwardsRecursive(unformattedNodes, formattedNodes, pPrevNode);
							}
						}
					}
				}
			}
		}
	}

	void CGraphViewFormatter::FormatNode(TGraphViewNodePtrVector& unformattedNodes, TGraphViewNodePtrVector& formattedNodes, const CGraphViewNodePtr& pNode, Gdiplus::PointF pos) const
	{
		TGraphViewNodePtrVector::iterator itUnformattedNode = std::find(unformattedNodes.begin(), unformattedNodes.end(), pNode);
		if(itUnformattedNode != unformattedNodes.end())
		{
			Gdiplus::RectF paintRect = pNode->GetPaintRect();
			paintRect.X = pos.X;
			paintRect.Y = pos.Y;
			for(const CGraphViewNodeConstPtr& pCollisionNode : formattedNodes)
			{
				if(pCollisionNode != pNode)
				{
					const Gdiplus::RectF collisionPaintRect = pCollisionNode->GetPaintRect();
					if(paintRect.IntersectsWith(collisionPaintRect))
					{
						paintRect.Y = collisionPaintRect.GetBottom() + m_settings.vertSpacing;
					}
				}
			}
			pNode->SetPos(Gdiplus::PointF(paintRect.X, paintRect.Y), m_settings.bSnapToGrid);
			unformattedNodes.erase(itUnformattedNode);
			formattedNodes.push_back(pNode);
		}
	}
}