// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __SelectionTranslator_h__
#define __SelectionTranslator_h__

#pragma once

/*
   A Leaf node name translator. Any leafs with a matching name will be translated.
   It supports disambiguation, if needed, with parent-qualified leaf names with : as separator.

   Example: Root:Combat:Shoot
 */

#include "BlockyXml.h"
#include "SelectionTree.h"

class SelectionTree;
class SelectionTranslator
{
public:
	bool LoadFromXML(const BlockyXmlBlocks::Ptr& blocks, const SelectionTree& selectionTree, const XmlNodeRef& rootNode,
	                 const char* scopeName, const char* fileName);

	const char* GetTranslation(const SelectionNodeID& nodeID) const;
private:
	struct QualifiedTranslation
	{
		std::vector<string> ancestors;
		string              target;
	};

	typedef std::map<SelectionNodeID, string> Translations;
	Translations m_translations;
};

#endif
