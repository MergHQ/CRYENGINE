// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry 
{
	namespace UDR
	{

		struct IHub
		{
			virtual ITreeManager&			GetTreeManager() = 0;
			virtual INodeStack&				GetNodeStack() = 0;
			virtual IRecursiveSyncObject&	GetRecursiveSyncObject() = 0;
		};

	}
}

