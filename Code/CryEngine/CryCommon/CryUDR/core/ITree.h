// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		struct ITree
		{
			virtual const INode& GetRootNode() const = 0;
			virtual void         RegisterListener(ITreeListener* pListenerToRegister) = 0;
			virtual void         UnregisterListener(ITreeListener* pListenerToUnregister) = 0;
			virtual void         RemoveNode(const INode& nodeToRemove) = 0;

		protected:

			~ITree() = default;  // not intended to get deleted via base class pointers
		};

	}
}