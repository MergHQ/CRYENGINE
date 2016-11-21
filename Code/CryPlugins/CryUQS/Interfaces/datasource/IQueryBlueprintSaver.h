// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace datasource
	{

		//===================================================================================
		//
		// IQueryBlueprintSaver
		//
		//===================================================================================

		struct IQueryBlueprintSaver
		{
			virtual                        ~IQueryBlueprintSaver() {}
			virtual bool                   SaveTextualQueryBlueprint(const core::ITextualQueryBlueprint& queryBlueprintToSave, shared::IUqsString& error) = 0;
		};

	}
}
