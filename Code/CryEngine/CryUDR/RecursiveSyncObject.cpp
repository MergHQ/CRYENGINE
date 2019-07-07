// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		void CRecursiveSyncObject::Lock()
		{
			m_critSect.Lock();
		}

		void CRecursiveSyncObject::Unlock()
		{
			m_critSect.Unlock();
		}

	}
}