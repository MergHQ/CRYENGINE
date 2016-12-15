// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace datasource
	{

		//===================================================================================
		//
		// IQueryBlueprintLoader
		//
		//===================================================================================

		struct IQueryBlueprintLoader
		{
			virtual                        ~IQueryBlueprintLoader() {}
			virtual bool                   LoadTextualQueryBlueprint(core::ITextualQueryBlueprint& out, shared::IUqsString& error) = 0;
		};

	}
}
