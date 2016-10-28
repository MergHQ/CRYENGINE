// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CEditorService
		//
		//===================================================================================

		class CEditorService : public IEditorService
		{
		public:
			virtual bool      LoadTextualQueryBlueprint(const char* queryBlueprintName, editor::IEditorServiceConsumer& callback, shared::IUqsString& error) const override;
			virtual bool      SaveTextualQueryBlueprint(const char* queryBlueprintName, editor::IEditorServiceConsumer& callback, shared::IUqsString& error) const override;
			virtual bool      CreateNewTextualQueryBlueprint(const char* queryBlueprintName, editor::IEditorServiceConsumer& callback, shared::IUqsString& error) const override;
			virtual bool      RemoveQueryBlueprint(const char* queryBlueprintName, shared::IUqsString& error) const override;
			virtual bool      ValidateTextualQueryBlueprint(editor::IEditorServiceConsumer& callback) const override;
		};

	}
}
