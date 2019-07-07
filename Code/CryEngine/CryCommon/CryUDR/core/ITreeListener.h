// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		struct ITreeListener
		{
			virtual void	OnBeforeRootNodeDeserialized() = 0;
			virtual void	OnNodeAdded(const INode& freshlyAddedNode) = 0;
			virtual void	OnBeforeNodeRemoved(const INode& nodeBeingRemoved) = 0;
			virtual void	OnAfterNodeRemoved(const void* pFormerAddressOfNode) = 0;

		protected:

			~ITreeListener() = default; // not intended to get deleted via base class pointers
		};

	}
}