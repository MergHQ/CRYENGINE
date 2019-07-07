// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry 
{
	namespace UDR
	{

		struct IString
		{
			virtual void		Set(const char* szString) = 0;
			virtual void		Format(const char* szFormat, ...) PRINTF_PARAMS(2, 3) = 0;
			virtual const char*	c_str() const = 0;

		protected:

			~IString() {}		// not intended to get deleted via base class pointers
		};

	}
}
