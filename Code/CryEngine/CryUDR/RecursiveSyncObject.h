// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		class CRecursiveSyncObject final : public IRecursiveSyncObject
		{
		public:

			// IRecursiveSyncObject
			virtual void		Lock() override;
			virtual void		Unlock() override;
			// ~IRecursiveSyncObject

		private:

			CryCriticalSection	m_critSect;
		};

	}
}