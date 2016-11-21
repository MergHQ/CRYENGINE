// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace datasource
	{
		//===================================================================================
		//
		// IEditorLibraryProvider
		//
		// UqsEditor uses this interface to interact with the library of queries.
		//
		//===================================================================================
		struct IEditorLibraryProvider
		{
			// TODO pavloi 2016.04.07: replace with functor? Is it OK to pass functors between DLL boundaries?
			struct IListQueriesVisitor
			{
				virtual void Visit(const char* szQueryName, const char* szQueryFilePath) = 0;
			};
			virtual void GetQueriesList(IListQueriesVisitor& visitor) = 0;

			virtual bool LoadTextualQueryBlueprint(const char* szQueryName, core::ITextualQueryBlueprint& outBlueprint, shared::IUqsString& error) = 0;
			virtual bool SaveTextualQueryBlueprint(const char* szQueryName, const core::ITextualQueryBlueprint& blueprintToSave, shared::IUqsString& error) = 0;

			virtual bool CreateNewTextualQueryBlueprint(const char* szQueryName, core::ITextualQueryBlueprint& outBlueprint, shared::IUqsString& error) = 0;
			virtual bool RemoveQueryBlueprint(const char* szQueryName, shared::IUqsString& error) = 0;
			// TODO pavloi 2016.04.08: rename query
			// TODO pavloi 2016.04.08: register listener for change monitoring
		};

	} // namespace interfaces
} // namespace uqs
