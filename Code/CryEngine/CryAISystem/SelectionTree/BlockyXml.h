// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __BlockyXml_h__
#define __BlockyXml_h__

#pragma once

/*
   This file implements a simple "lazy" XML reference resolver.

   It works by collecting all the available reference-able XML nodes
   and as a first step. Then it will resolve references while iterating through nodes.

   It supports 1 level of scoping, meaning it will look for references first in the specified scope
   and then on the global scope shall that fail.
 */

class BlockyXmlBlocks :
	public _reference_target_t
{
public:
	typedef _smart_ptr<BlockyXmlBlocks> Ptr;

	struct Block
	{
		Block()
		{
		}

		Block(const char* _fileName, const XmlNodeRef& _blockNode)
			: fileName(_fileName)
			, blockNode(_blockNode)
		{
		}

		string     fileName;
		XmlNodeRef blockNode;
	};

	bool  AddBlock(const char* scopeName, const char* name, const XmlNodeRef& node, const char* fileName = "<unknownFile>");
	Block GetBlock(const char* scopeName, const char* blockName);

private:
	typedef std::map<string, Block>  Blocks;
	typedef std::map<string, Blocks> BlockScopes;
	BlockScopes m_scopes;
};

class BlockyXmlNodeRef
{
public:
	BlockyXmlNodeRef();
	BlockyXmlNodeRef(const BlockyXmlBlocks::Ptr& blocks, const char* scopeName, const XmlNodeRef& rootNode, const char* fileName = "<unknownFile>");
	BlockyXmlNodeRef(const BlockyXmlBlocks::Ptr& blocks, const char* scopeName, const BlockyXmlBlocks::Block& block);
	BlockyXmlNodeRef(const BlockyXmlNodeRef& other);

	void       first();
	XmlNodeRef next();

private:
	XmlNodeRef                        m_rootNode;
	string                            m_scopeName;
	string                            m_fileName;
	BlockyXmlBlocks::Ptr              m_blocks;
	std::unique_ptr<BlockyXmlNodeRef> m_currRef;
	uint16                            m_currIdx;
};

#endif
