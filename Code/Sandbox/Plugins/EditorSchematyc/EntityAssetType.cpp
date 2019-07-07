// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityAssetType.h"

#include "SchematycUtils.h"

#include <AssetSystem/AssetEditor.h>
#include <AssetSystem/Browser/AssetBrowserDialog.h>
#include <AssetSystem/EditableAsset.h>
#include <Controls/QuestionDialog.h>
#include <QtUtil.h>

namespace CrySchematycEditor {

REGISTER_ASSET_TYPE(CEntityAssetType)

class CBaseClassDialog
{
public:
	CBaseClassDialog() {}

	Schematyc::SElementId GetResult()
	{
		ICrySchematycCore* pSchematycCore = gEnv->pSchematyc;
		if (pSchematycCore)
		{
			Schematyc::IScriptRegistry& scriptRegistry = pSchematycCore->GetScriptRegistry();

			CQuestionDialog dialog;
			dialog.SetupQuestion("Schematyc Entity", "Do you want to use another Schematyc Entity as base?", QDialogButtonBox::Yes | QDialogButtonBox::No);

			QDialogButtonBox::StandardButton result = dialog.Execute();
			if (result == QDialogButtonBox::Yes)
			{
				Schematyc::SElementId baseClassId;

				CAsset* pAsset = CAssetBrowserDialog::OpenSingleAssetForTypes(std::vector<string>(1, CEntityAssetType::TypeName()));
				if (pAsset)
				{
					const size_t fileCount = pAsset->GetFilesCount();
					CRY_ASSERT_MESSAGE(fileCount > 0, "Schematyc Editor: Asset '%s' has no files.", pAsset->GetName());

					const stack_string szFile = PathUtil::GetGameFolder() + "/" + string(pAsset->GetFile(0));
					Schematyc::IScript* pScript = scriptRegistry.GetScriptByFileName(szFile);
					if (pScript)
					{
						Schematyc::IScriptElement* pRootElement = pScript->GetRoot();
						if (pRootElement->GetType() == Schematyc::EScriptElementType::Class)
						{
							const Schematyc::IScriptClass* pScriptClass = static_cast<Schematyc::IScriptClass*>(pRootElement);
							return Schematyc::SElementId(Schematyc::EDomain::Script, pScriptClass->GetGUID());
						}
					}

					CRY_ASSERT_MESSAGE(pScript, "Selected script not found or of an invalid type.");
				}
			}
			else
			{
				Schematyc::SElementId baseClassId;
				auto visitEnvClass = [&baseClassId](const Schematyc::IEnvClass& envClass) -> Schematyc::EVisitStatus
				{
					baseClassId = Schematyc::SElementId(Schematyc::EDomain::Env, envClass.GetGUID());
					return Schematyc::EVisitStatus::Stop;
				};
				Schematyc::IScriptViewPtr pScriptView = gEnv->pSchematyc->CreateScriptView(scriptRegistry.GetRootElement().GetGUID());
				pScriptView->VisitEnvClasses(visitEnvClass);

				return baseClassId;
			}
		}

		return Schematyc::SElementId();
	}
};

bool CEntityAssetType::OnCreate(INewAsset& asset, const SCreateParams* pCreateParams) const
{
	const string dataFilePath = PathUtil::RemoveExtension(asset.GetMetadataFile());
	const string assetName = asset.GetName();

	CBaseClassDialog baseClassDialog;
	const Schematyc::SElementId baseClassId = baseClassDialog.GetResult();
	if (baseClassId.guid != CryGUID::Null() && baseClassId.domain != Schematyc::EDomain::Unknown)
	{
		// TODO: Actually the backend should ensure that the name is valid!
		Schematyc::CStackString uniqueAssetName(assetName.c_str());
		MakeScriptElementNameUnique(uniqueAssetName);
		// ~TODO

		Schematyc::IScriptRegistry& scriptRegistry = gEnv->pSchematyc->GetScriptRegistry();
		Schematyc::IScriptClass* pClass = scriptRegistry.AddClass(uniqueAssetName.c_str(), baseClassId, PathUtil::RemoveExtension(dataFilePath).c_str());
		if (pClass)
		{
			if (Schematyc::IScript* pScript = pClass->GetScript())
			{
				asset.AddFile(dataFilePath.c_str());

				scriptRegistry.SaveScript(*pScript);
				gEnv->pSchematyc->GetCompiler().CompileDependencies(pScript->GetRoot()->GetGUID());
				return true;
			}
		}
	}

	return false;
}

CAssetEditor* CEntityAssetType::Edit(CAsset* pAsset) const
{
	return CAssetEditor::OpenAssetForEdit("Schematyc Editor (Experimental)", pAsset);
}

bool CEntityAssetType::RenameAsset(CAsset* pAsset, const char* szNewName) const
{
	if (pAsset)
	{
		Schematyc::IScript* pScript = GetScript(*pAsset);
		if (pScript)
		{
			CryPathString prevName = pAsset->GetName();
			if (CAssetType::RenameAsset(pAsset, szNewName))
			{
				CryPathString filePath = pScript->GetFilePath();
				CryPathString path;
				CryPathString file;
				CryPathString ext;
				PathUtil::Split(filePath, path, file, ext);
				file = szNewName;
				filePath = PathUtil::Make(path, file, ext);

				gEnv->pSchematyc->GetScriptRegistry().OnScriptRenamed(*pScript, filePath);
				return true;
			}
		}
	}

	return false;
}

void CEntityAssetType::PreDeleteAssetFiles(const CAsset& asset) const
{
	Schematyc::IScript* pScript = GetScript(asset);
	if (pScript)
	{
		gEnv->pSchematyc->GetScriptRegistry().RemoveElement(pScript->GetRoot()->GetGUID());
	}
}

string CEntityAssetType::GetObjectFilePath(const CAsset* pAsset) const
{
	if (pAsset)
	{
		const size_t fileCount = pAsset->GetFilesCount();
		CRY_ASSERT_MESSAGE(fileCount == 1, "Asset '%s' has an unexpected amount of script files.", pAsset->GetName());
		if (fileCount > 0)
		{
			string scriptName = PathUtil::RemoveExtension(PathUtil::RemoveExtension(pAsset->GetFile(0)));
			scriptName.replace("/", "::");
			scriptName.MakeLower();
			return string().Format("schematyc::%s", scriptName.c_str());
		}
	}
	return string();
}

CryIcon CEntityAssetType::GetIconInternal() const
{
	return CryIcon("icons:schematyc/assettype_entity.ico");
}

Schematyc::IScript* CEntityAssetType::GetScript(const CAsset& asset) const
{
	ICrySchematycCore* pSchematycCore = gEnv->pSchematyc;
	if (pSchematycCore)
	{
		Schematyc::IScriptRegistry& scriptRegistry = pSchematycCore->GetScriptRegistry();
		const size_t fileCount = asset.GetFilesCount();
		CRY_ASSERT_MESSAGE(fileCount == 1, "Asset '%s' has an unexpected amount of script files.", asset.GetName());
		if (fileCount > 0)
		{
			const stack_string szFile = PathUtil::GetGameFolder() + "/" + string(asset.GetFile(0));
			return scriptRegistry.GetScriptByFileName(szFile);
		}
	}
	return nullptr;
}

}
