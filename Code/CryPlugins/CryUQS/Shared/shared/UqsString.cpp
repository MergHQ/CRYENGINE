// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "UqsString.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace shared
	{

		//===================================================================================
		//
		// CUqsString
		//
		//===================================================================================

		CUqsString::CUqsString()
			: m_message()
		{
			// nothing
		}

		CUqsString::CUqsString(const char* szString)
			: m_message(szString)
		{
			// nothing
		}

		void CUqsString::Set(const char* szString)
		{
			m_message = szString;
		}

		void CUqsString::Format(const char* fmt, ...)
		{
			va_list ap;
			va_start(ap, fmt);
			m_message.FormatV(fmt, ap);
			va_end(ap);
		}

		const char* CUqsString::c_str() const
		{
			return m_message.c_str();
		}

	}
}
