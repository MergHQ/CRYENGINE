// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace DataSource
	{

		struct ISyntaxErrorCollector;       // below
		class CSyntaxErrorCollectorDeleter; // below

		//===================================================================================
		//
		// SyntaxErrorCollectorUniquePtr
		//
		//===================================================================================

		typedef std::unique_ptr<ISyntaxErrorCollector, CSyntaxErrorCollectorDeleter> SyntaxErrorCollectorUniquePtr;

		//===================================================================================
		//
		// ISyntaxErrorCollector
		//
		// - provides a way to notify about errors when resolving a query blueprint from its textual representation into the "in-memory" representation
		// - the code that is responsible for loading a blueprint from a data source into the textual representation will attach specific instances of this interface to each of the ITextual* objects in the hierarchy
		// - depending on where the original data came from (e. g. XML, UI, ...), this class may add further information (e. g. XML line number) or trigger something in an editor UI to highlight the error
		//
		//===================================================================================

		struct ISyntaxErrorCollector
		{
			virtual                        ~ISyntaxErrorCollector() {}
			virtual void                   AddErrorMessage(const char* fmt, ...) PRINTF_PARAMS(2, 3) = 0;
		};

		//===================================================================================
		//
		// CSyntaxErrorCollectorDeleter
		//
		//===================================================================================

		class CSyntaxErrorCollectorDeleter
		{
		public:
			explicit                       CSyntaxErrorCollectorDeleter();
			explicit                       CSyntaxErrorCollectorDeleter(void (*deleteFunc)(ISyntaxErrorCollector* pSyntaxErrorCollectorToDelete));
			void                           operator()(ISyntaxErrorCollector* pSyntaxErrorCollectorToDelete);

		private:
			void                           (*m_deleteFunc)(ISyntaxErrorCollector* pSyntaxErrorCollectorToDelete);
		};

		inline CSyntaxErrorCollectorDeleter::CSyntaxErrorCollectorDeleter()
			: m_deleteFunc(nullptr)
		{}

		inline CSyntaxErrorCollectorDeleter::CSyntaxErrorCollectorDeleter(void (*deleteFunc)(ISyntaxErrorCollector* pSyntaxErrorCollectorToDelete))
			: m_deleteFunc(deleteFunc)
		{}

		inline void CSyntaxErrorCollectorDeleter::operator()(ISyntaxErrorCollector* pSyntaxErrorCollectorToDelete)
		{
			assert(m_deleteFunc);
			(*m_deleteFunc)(pSyntaxErrorCollectorToDelete);
		}

	}
}
