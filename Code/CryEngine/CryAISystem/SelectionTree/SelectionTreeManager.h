// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __SelectionTreeManager_h__
#define __SelectionTreeManager_h__

#pragma once

/*
   This file implements a container and loader for selection trees.
 */

#include <CryAISystem/ISelectionTreeManager.h>

#include "BlockyXml.h"
#include "SelectionTreeTemplate.h"

class CSelectionTreeManager :
	public ISelectionTreeManager
{
public:
	virtual ~CSelectionTreeManager(){}
	virtual void                 Reload();
	void                         ScanFolder(const char* folderName);

	void                         Reset();

	SelectionTreeTemplateID      GetTreeTemplateID(const char* treeName) const;
	bool                         HasTreeTemplate(const SelectionTreeTemplateID& templateID) const;
	const SelectionTreeTemplate& GetTreeTemplate(const SelectionTreeTemplateID& templateID) const;

	// ISelectionTreeManager
	virtual uint32                  GetSelectionTreeCount() const;
	virtual uint32                  GetSelectionTreeCountOfType(const char* typeName) const;

	virtual const char*             GetSelectionTreeName(uint32 index) const;
	virtual const char*             GetSelectionTreeNameOfType(const char* typeName, uint32 index) const;

	virtual ISelectionTreeDebugger* GetDebugger() const;
	//~ISelectionTreeManager

private:
	void ResetBehaviorSelectionTreeOfAIActorOfType(unsigned short int nType);

	void DiscoverFolder(const char* folderName);
	bool Discover(const char* fileName);
	bool LoadBlocks(const XmlNodeRef& blocksNode, const char* scope, const char* fileName);
	bool LoadBlock(const XmlNodeRef& blockNode, const char* scope, const char* fileName);
	bool DiscoverBlocks(const XmlNodeRef& rootNode, const char* scope, const char* fileName);

	bool LoadFileNode(const XmlNodeRef& rootNode, const char* fileName);
	bool LoadTreeTemplate(const XmlNodeRef& rootNode, const char* fileName);

	typedef std::map<SelectionTreeTemplateID, SelectionTreeTemplate> TreeTemplates;
	TreeTemplates m_templates;

	typedef std::multimap<string, SelectionTreeTemplateID> TreeTemplateTypes;
	TreeTemplateTypes    m_templateTypes;

	BlockyXmlBlocks::Ptr m_blocks;

	struct FileNode
	{
		FileNode(const char* file, const XmlNodeRef& node)
			: fileName(file)
			, rootNode(node)
		{
		}

		string     fileName;
		XmlNodeRef rootNode;
	};

	typedef std::vector<FileNode> FileNodes;
	FileNodes m_fileNodes;

	typedef std::vector<string> FolderNames;
	FolderNames m_folderNames;
};

#endif
