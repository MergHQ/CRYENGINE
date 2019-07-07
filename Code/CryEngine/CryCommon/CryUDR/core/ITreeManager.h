// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		struct ITreeManager
		{
			enum class ETreeIndex
			{
				Live,
				Deserialized,

				// keep this the last entry
				Count
			};

			virtual bool   SerializeLiveTree(const char* szXmlFilePath, IString& outError) const = 0;
			virtual bool   DeserializeTree(const char* szXmlFilePath, IString& outError) = 0;
			virtual ITree& GetTree(ETreeIndex treeIndex) = 0;

		protected:

			~ITreeManager() = default; // not intended to get deleted via base class pointers
		};

	}
}