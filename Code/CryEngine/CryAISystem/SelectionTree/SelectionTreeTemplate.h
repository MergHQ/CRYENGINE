// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __SelectionTreeTemplate_h__
#define __SelectionTreeTemplate_h__

#pragma once

/*
   This file implements a simple container for all the concepts required to run a selection tree.
 */

#include "SelectionTree.h"
#include "SelectionTranslator.h"
#include "SelectionSignalVariables.h"

class SelectionTreeTemplate
{
public:
	bool LoadFromXML(const SelectionTreeTemplateID& templateID, const BlockyXmlBlocks::Ptr& blocks, const XmlNodeRef& rootNode,
	                 const char* fileName);

	const SelectionTree&                 GetSelectionTree() const;
	const SelectionVariableDeclarations& GetVariableDeclarations() const;
	const SelectionTranslator&           GetTranslator() const;
	const SelectionSignalVariables&      GetSignalVariables() const;

	const char*                          GetName() const;
	bool                                 Valid() const;

private:
	SelectionTree                 m_selectionTree;
	SelectionVariableDeclarations m_variableDecls;
	SelectionTranslator           m_translator;
	SelectionSignalVariables      m_signalVariables;

	string                        m_name;
};

#endif
