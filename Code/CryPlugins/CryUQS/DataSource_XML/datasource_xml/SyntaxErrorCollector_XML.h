// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace datasource_xml
	{

		class CXMLDataErrorCollector;    // below

		//===================================================================================
		//
		// CSyntaxErrorCollector_XML
		//
		//===================================================================================

		class CSyntaxErrorCollector_XML : public datasource::ISyntaxErrorCollector
		{
		public:
			explicit                                   CSyntaxErrorCollector_XML(int xmlLineNumber, const std::shared_ptr<CXMLDataErrorCollector>& dataErrorCollector);
			virtual void                               AddErrorMessage(const char* fmt, ...) override;

		private:
			                                           UQS_NON_COPYABLE(CSyntaxErrorCollector_XML);

			int                                        m_xmlLineNumber;
			std::shared_ptr<CXMLDataErrorCollector>    m_dataErrorCollector;
		};

		//===================================================================================
		//
		// CXMLDataErrorCollector
		//
		//===================================================================================

		class CXMLDataErrorCollector
		{
		public:
			explicit                              CXMLDataErrorCollector();

			void                                  AddError(const char* error);
			size_t                                GetErrorCount() const;
			const char*                           GetError(size_t index) const;

		private:
			                                      UQS_NON_COPYABLE(CXMLDataErrorCollector);

		private:
			std::vector<string>                   m_errorCollection;
		};

	}
}
