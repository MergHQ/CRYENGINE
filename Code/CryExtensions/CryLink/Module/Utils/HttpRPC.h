// Copyright 2001-2019 Crytek GmbH. All rights reserved.

#pragma once

namespace CryLinkService 
{
	namespace ValueConversion
	{
		static string EscapeXMLReserved(const string& str)
		{
			string escapedString;
			escapedString.reserve(str.length() * 2);
			for(char ch : str)
			{
				switch(ch)
				{
				case '&': 
					escapedString.append("&amp;"); 
					break;
				case '<': 
					escapedString.append("&lt;"); 
					break;
				case '>': 
					escapedString.append("&gt;"); 
					break;
				case '\\': 
					escapedString.append("&quot;"); 
					break;
				case '\'': 
					escapedString.append("&apos;"); 
					break;
				case '%': 
					escapedString.append("&#37;"); 
					break;
				default: 
					escapedString.append(1, ch);
					break;
				}
			}

			return escapedString;
		}

		template <typename T>
		const char* ToXmlRPCValue(stack_string& buffer, const T& value);

		template <>
		const char* ToXmlRPCValue(stack_string& buffer, const string& value)
		{
			buffer.Format("<string>%s</string>", EscapeXMLReserved(value).c_str());
			return buffer.c_str();
		}

		template <>
		const char* ToXmlRPCValue(stack_string& buffer, const bool& value)
		{
			buffer.Format("<boolean>%d</boolean>", value ? 1 : 0);
			return buffer.c_str();
		}

		template <>
		const char* ToXmlRPCValue(stack_string& buffer, const int32& value)
		{
			buffer.Format("<i4>%d</i4>", value);
			return buffer.c_str();
		}

		template <>
		const char* ToXmlRPCValue(stack_string& buffer, const int64& value)
		{
			buffer.Format("<i8>%d</i8>", value);
			return buffer.c_str();
		}
	}

	template <typename TResponseValue>
	struct SRPCResponse
	{
		ISimpleHttpServer& httpServer;
		TResponseValue response;
		const int32 connectionId;
		const ISimpleHttpServer::EContentType contentType;
		ISimpleHttpServer::EStatusCode statusCode;
		bool bKeepAlive;

		SRPCResponse(ISimpleHttpServer& _httpServer, int32 _connectionId)
			: httpServer(_httpServer)
			, connectionId(_connectionId)
			, statusCode(ISimpleHttpServer::eSC_Okay)
			, contentType(ISimpleHttpServer::eCT_XML)
			, bKeepAlive(true)
		{}

		void Send()
		{
			stack_string content;
			stack_string responseString;
			content.Format(
				"<?xml version=\"1.0\"?><methodResponse><params><param><value>%s</value></param></params></methodResponse>", 
				ValueConversion::ToXmlRPCValue(responseString, response)
			);

			httpServer.SendResponse(connectionId, 
				statusCode, 
				contentType, 
				content.c_str(),
				!bKeepAlive
			);
		}	
	};

}