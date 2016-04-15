// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __SelectionContext_h__
#define __SelectionContext_h__

#pragma once

/*
   This is the evaluation context for nodes.
 */

#include "SelectionVariables.h"
#include "SelectionTreeNode.h"

class SelectionContext
{
public:
	SelectionContext(SelectionTreeNodes& nodes, const SelectionNodeID& currentNodeID,
	                 const SelectionVariables& variables)
		: m_nodes(nodes)
		, m_currentNodeID(currentNodeID)
		, m_variables(variables)
	{
	}

	const SelectionVariables& GetVariables() const
	{
		return m_variables;
	}

	SelectionTreeNode& GetNode(const SelectionNodeID& nodeID) const
	{
		assert(nodeID > 0 && nodeID <= m_nodes.size());
		return m_nodes[nodeID - 1];
	}

	const SelectionNodeID& GetCurrentNodeID() const
	{
		return m_currentNodeID;
	}

private:
	SelectionTreeNodes&       m_nodes;
	SelectionNodeID           m_currentNodeID;

	const SelectionVariables& m_variables;
};

#endif
