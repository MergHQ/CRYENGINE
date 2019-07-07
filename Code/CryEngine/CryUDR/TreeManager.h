// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		class CTreeManager final : public ITreeManager
		{
		public:

			// ITreeManager
			virtual bool	SerializeLiveTree(const char* szXmlFilePath, IString& outError) const override;
			virtual bool	DeserializeTree(const char* szXmlFilePath, IString& outError) override;
			virtual CTree&	GetTree(ETreeIndex treeIndex) override;
			// ~ITreeManager

		private:

			CTree			m_trees[static_cast<size_t>(ETreeIndex::Count)];
		};

	}
}