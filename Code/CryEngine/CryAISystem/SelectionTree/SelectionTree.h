// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __SelectionTree_h__
#define __SelectionTree_h__

#pragma once

/*
   This file implements the runtime portion of the selection tree.
   It is also used as the tree template, and new trees are just assignments of this one.
 */

#include "BlockyXml.h"
#include "SelectionTreeNode.h"
#include "SelectionVariables.h"

typedef uint32 SelectionTreeTemplateID;

class SelectionTreeTemplate;
class SelectionTree
{
	typedef SelectionNodeID NodeID;
public:
	SelectionTree();
	SelectionTree(const SelectionTreeTemplateID& templateID);
	SelectionTree(const SelectionTree& selectionTree);

	void                           Clear();
	void                           Reset();

	const SelectionNodeID&         Evaluate(const SelectionVariables& variables);

	SelectionNodeID                AddNode(const SelectionTreeNode& node);
	SelectionTreeNode&             GetNode(const NodeID& nodeID);
	const SelectionTreeNode&       GetNode(const NodeID& nodeID) const;
	const SelectionNodeID&         GetCurrentNodeID() const;

	const SelectionTreeTemplateID& GetTemplateID() const;
	const SelectionTreeTemplate&   GetTemplate() const;

	uint32                         GetNodeCount() const;
	const SelectionTreeNode& GetNodeAt(uint32 index) const;

	void                     ReserveNodes(uint32 nodeCount);
	bool                     Empty() const;
	void                     Swap(SelectionTree& other);
	void                     Validate();

	void                     Serialize(TSerialize ser);

	void                     DebugDraw() const;

private:
	SelectionTreeTemplateID m_templateID;
	SelectionTreeNodes      m_nodes;
	NodeID                  m_currentNodeID;
};

#endif
