// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Core
	{

		//===================================================================================
		//
		// CEditorService
		//
		//===================================================================================

		class CEditorService : public IEditorService
		{
		public:
			virtual bool      LoadTextualQueryBlueprint(const char* queryBlueprintName, Editor::IEditorServiceConsumer& callback, Shared::IUqsString& error) const override;
			virtual bool      SaveTextualQueryBlueprint(const char* queryBlueprintName, Editor::IEditorServiceConsumer& callback, Shared::IUqsString& error) const override;
			virtual bool      CreateNewTextualQueryBlueprint(const char* queryBlueprintName, Editor::IEditorServiceConsumer& callback, Shared::IUqsString& error) const override;
			virtual bool      RemoveQueryBlueprint(const char* queryBlueprintName, Shared::IUqsString& error) const override;
			virtual bool      ValidateTextualQueryBlueprint(Editor::IEditorServiceConsumer& callback) const override;
		};

	}
}
