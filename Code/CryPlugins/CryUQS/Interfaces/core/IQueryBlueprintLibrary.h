// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// IQueryBlueprintLibrary
		//
		//===================================================================================

		struct IQueryBlueprintLibrary
		{
			enum class ELoadAndStoreOverwriteBehavior
			{
				OverwriteExisting,
				KeepExisting
			};

			enum class ELoadAndStoreResult
			{
				StoredFromScratch,              // there was no query blueprint with the same name in the library before
				OverwrittenExistingOne,         // there was a query blueprint with the same name already, but it got overwritten now
				PreservedExistingOne,           // there was a query blueprint with the same name already, and it was preserved
				ExceptionOccurred,              // the query blueprint could not be loaded from the data source at all, and hasn't therefore stored in the library
			};

			virtual                             ~IQueryBlueprintLibrary() {}
			virtual ELoadAndStoreResult         LoadAndStoreQueryBlueprint(ELoadAndStoreOverwriteBehavior overwriteBehavior, datasource::IQueryBlueprintLoader& loader, shared::IUqsString& error) = 0;
			virtual bool                        RemoveStoredQueryBlueprint(const char* szQueryBlueprintName, shared::IUqsString& error) = 0;
		};

	}
}
