// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		SERIALIZATION_ENUM_BEGIN(ELogMessageType, "")
		SERIALIZATION_ENUM(ELogMessageType::Information, "Information", "")
		SERIALIZATION_ENUM(ELogMessageType::Warning, "Warning", "")
		SERIALIZATION_ENUM_END()

		CLogMessageCollection::SMessage::SMessage()
			: text()
			, type(ELogMessageType::Information)
		{}

		CLogMessageCollection::SMessage::SMessage(const char* _szText, ELogMessageType _type)
			: text(_szText)
			, type(_type)
		{}

		void CLogMessageCollection::SMessage::Serialize(Serialization::IArchive& ar)
		{
			ar(text, "text");
			ar(type, "type");
		}

		void CLogMessageCollection::LogInformation(const char* szFormat, ...)
		{
			stack_string text;
			va_list args;
			va_start(args, szFormat);
			text.FormatV(szFormat, args);
			va_end(args);
			m_messages.emplace_back(text.c_str(), ELogMessageType::Information);
		}

		void CLogMessageCollection::LogWarning(const char* szFormat, ...)
		{
			stack_string text;
			va_list args;
			va_start(args, szFormat);
			text.FormatV(szFormat, args);
			va_end(args);
			m_messages.emplace_back(text.c_str(), ELogMessageType::Warning);
		}

//#error next steps: (1) test: add some log messages via game code and save them, (2) editor: show log messages of the selected nodes via an additional text box

		void CLogMessageCollection::Visit(INode::ILogMessageVisitor& visitor) const
		{
			for (const SMessage& msg : m_messages)
			{
				visitor.OnLogMessageVisited(msg.text.c_str(), msg.type);
			}
		}

		void CLogMessageCollection::Serialize(Serialization::IArchive& ar)
		{
			ar(m_messages, "messages");
		}

	}
}