// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <vector>

class CEntityObject;
class CHyperFlowGraph;

namespace FlowGraphHelpers
{
// Description:
//     Get human readable name for the FlowGraph
//     incl. group, entity, AI action
// Arguments:
//     pFlowGraph - Ptr to flowgraph
//     outName    - CString with human name
// Return Value:
//     none
void GetHumanName(CHyperFlowGraph* pFlowGraph, CString& outName);

// Description:
//     Get human readable name for the FlowGraph
//     incl. group, entity, AI action
// Arguments:
//     pFlowGraph - Ptr to flowgraph
//     outName    - string with human name
// Return Value:
//     none
inline void GetHumanName(CHyperFlowGraph* pFlowGraph, string& outName) // for CString conversion
{
	CString cstr = outName.GetString();
	GetHumanName(pFlowGraph, cstr);
	outName = cstr.GetString();
}

// Description:
//     Get all flowgraphs an entity is used in
// Arguments:
//     pEntity            - Entity
//     outFlowGraphs      - Vector of all flowgraphs (sorted by HumanName)
//     outEntityFlowGraph - If the entity is owner of a flowgraph this is the pointer to
// Return Value:
//     none
void FindGraphsForEntity(const CEntityObject* pEntity, std::vector<CHyperFlowGraph*>& outFlowGraphs, CHyperFlowGraph*& outEntityFlowGraph);

void OpenFlowGraphView(CHyperFlowGraph* pFlowGraph, string selectNode);

void ListFlowGraphsForEntity(CEntityObject* pEntity);
};
