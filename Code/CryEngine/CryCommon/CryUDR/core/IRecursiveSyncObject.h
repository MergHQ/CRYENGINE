// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		struct IRecursiveSyncObject
		{
			virtual void Lock() = 0;
			virtual void Unlock() = 0;

		protected:

			~IRecursiveSyncObject() = default; // not intended to get deleted via base class pointers
		};

	}
}