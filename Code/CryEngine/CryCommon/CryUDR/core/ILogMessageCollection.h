// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		enum class ELogMessageType
		{
			Information,
			Warning,

			Count
		};

		struct ILogMessageCollection
		{
			virtual void    LogInformation(const char* szFormat, ...) PRINTF_PARAMS(2, 3) = 0;
			virtual void    LogWarning(const char* szFormat, ...) PRINTF_PARAMS(2, 3) = 0;

		protected:

			~ILogMessageCollection() {} // not intended to get deleted via base class pointers
		};

	}
}