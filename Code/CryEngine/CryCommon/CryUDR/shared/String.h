// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		class CString : public IString
		{
		public:

			explicit			CString();
			explicit			CString(const char* szString);

			// IString
			virtual void		Set(const char* szString) override;
			virtual void		Format(const char* szFormat, ...) override PRINTF_PARAMS(2, 3);
			virtual const char*	c_str() const override;
			// ~IString

		private:

			string				m_text;
		};

		inline CString::CString()
			: m_text()
		{
			// nothing
		}

		inline CString::CString(const char* szString)
			: m_text(szString)
		{
			// nothing
		}

		inline void CString::Set(const char* szString)
		{
			// TODO: guard against assignment from own underlying string
			m_text = szString;
		}

		inline void CString::Format(const char* szFormat, ...)
		{
			va_list args;
			va_start(args, szFormat);
			m_text.FormatV(szFormat, args);
			va_end(args);
		}

		inline const char* CString::c_str() const
		{
			return m_text.c_str();
		}

	}
}