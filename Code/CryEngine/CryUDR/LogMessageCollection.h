// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		class CLogMessageCollection final : public ILogMessageCollection
		{
		private:

			struct SMessage
			{
				                   SMessage();  // default ctor required for yasli serialization
				explicit           SMessage(const char* _szText, ELogMessageType _type);
				void               Serialize(Serialization::IArchive& ar);

				string             text;
				ELogMessageType    type;
			};

		public:

			// ILogMessageCollection
			virtual void           LogInformation(const char* szFormat, ...) override PRINTF_PARAMS(2, 3);
			virtual void           LogWarning(const char* szFormat, ...) override PRINTF_PARAMS(2, 3);
			// ~ILogMessageCollection

			void                   Visit(INode::ILogMessageVisitor& visitor) const;
			void                   Serialize(Serialization::IArchive& ar);

		private:

			std::vector<SMessage>  m_messages;
		};

	}
}