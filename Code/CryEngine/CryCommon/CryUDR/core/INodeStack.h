// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		struct INodeStack
		{
			virtual void   PushNode(INode& nodeToPush) = 0;
			virtual INode& GetTopNode() = 0;
			virtual void   PopNode() = 0;

		protected:

			~INodeStack() = default; // not intended to get deleted via base class pointers
		};

	}
}