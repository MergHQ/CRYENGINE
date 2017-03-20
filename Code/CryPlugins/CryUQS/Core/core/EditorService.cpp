// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EditorService.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Core
	{

		bool CEditorService::LoadTextualQueryBlueprint(const char* queryBlueprintName, Editor::IEditorServiceConsumer& callback, Shared::IUqsString& error) const
		{
			if (DataSource::IEditorLibraryProvider* pEditorLibraryProvider = g_hubImpl->GetEditorLibraryProvider())
			{
				CTextualQueryBlueprint loadedTextualQueryBlueprint;
				const bool loadedSuccessfully = pEditorLibraryProvider->LoadTextualQueryBlueprint(queryBlueprintName, loadedTextualQueryBlueprint, error);
				if (loadedSuccessfully)
				{
					callback.OnTextualQueryBlueprintLoaded(loadedTextualQueryBlueprint);
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				error.Format("There's no IEditorLibraryProvider installed in the IHub");
				return false;
			}
		}

		bool CEditorService::SaveTextualQueryBlueprint(const char* queryBlueprintName, Editor::IEditorServiceConsumer& callback, Shared::IUqsString& error) const
		{
			if (DataSource::IEditorLibraryProvider* pEditorLibraryProvider = g_hubImpl->GetEditorLibraryProvider())
			{
				CTextualQueryBlueprint textualQueryBlueprint;
				callback.OnTextualQueryBlueprintGettingSaved(textualQueryBlueprint);

				const bool savedSuccessfully = pEditorLibraryProvider->SaveTextualQueryBlueprint(queryBlueprintName, textualQueryBlueprint, error);
				return savedSuccessfully;
			}
			else
			{
				error.Format("There's no IEditorLibraryProvider installed in the IHub");
				return false;
			}
		}

		bool CEditorService::CreateNewTextualQueryBlueprint(const char* queryBlueprintName, Editor::IEditorServiceConsumer& callback, Shared::IUqsString& error) const
		{
			if (DataSource::IEditorLibraryProvider* pEditorLibraryProvider = g_hubImpl->GetEditorLibraryProvider())
			{
				CTextualQueryBlueprint textualQueryBlueprint;
				const bool createdSuccessfully = pEditorLibraryProvider->CreateNewTextualQueryBlueprint(queryBlueprintName, textualQueryBlueprint, error);
				if (createdSuccessfully)
				{
					callback.OnTextualQueryBlueprintLoaded(textualQueryBlueprint);
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				error.Format("There's no IEditorLibraryProvider installed in the IHub");
				return false;
			}
		}

		bool CEditorService::RemoveQueryBlueprint(const char* queryBlueprintName, Shared::IUqsString& error) const
		{
			if (DataSource::IEditorLibraryProvider* pEditorLibraryProvider = g_hubImpl->GetEditorLibraryProvider())
			{
				return pEditorLibraryProvider->RemoveQueryBlueprint(queryBlueprintName, error);
			}
			else
			{
				error.Format("There's no IEditorLibraryProvider installed in the IHub");
				return false;
			}
		}

		bool CEditorService::ValidateTextualQueryBlueprint(Editor::IEditorServiceConsumer& callback) const
		{
			CTextualQueryBlueprint textualQueryBlueprint;
			callback.OnTextualQueryBlueprintGettingValidated(textualQueryBlueprint);

			CQueryBlueprint actualQueryBlueprintForValidation;
			return actualQueryBlueprintForValidation.Resolve(textualQueryBlueprint);   // this will add error messages to all attached ISyntaxErrorCollectors
		}

	}
}
