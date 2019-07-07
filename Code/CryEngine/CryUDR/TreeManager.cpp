// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		bool CTreeManager::SerializeLiveTree(const char* szXmlFilePath, IString& outError) const
		{
			return m_trees[static_cast<size_t>(ETreeIndex::Live)].Save(szXmlFilePath, outError);
		}

		bool CTreeManager::DeserializeTree(const char* szXmlFilePath, IString& outError)
		{
			return m_trees[static_cast<size_t>(ETreeIndex::Deserialized)].Load(szXmlFilePath, outError);
		}

		CTree& CTreeManager::GetTree(ETreeIndex treeIndex)
		{
			return m_trees[static_cast<size_t>(treeIndex)];
		}

	}
}