// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CharacterDefinitionType.h"
#include "SandboxPlugin.h"
#include "ImporterUtil.h"
#include "RcCaller.h"
#include "AnimationHelpers/AnimationHelpers.h"

#include <AssetSystem/Asset.h>
#include <AssetSystem/EditableAsset.h>
#include <AssetSystem/AssetImportContext.h>
#include <FilePathUtil.h>

REGISTER_ASSET_TYPE(CCharacterDefinitionType)

namespace MeshImporter
{

QString CreateCHPARAMS(const QString& path);

} // namespace MeshImporter

string CCharacterDefinitionType::GetTargetFilename(const string& sourceFilename) const
{
	return PathUtil::ReplaceExtension(sourceFilename, "cdf");
}

CryIcon CCharacterDefinitionType::GetIcon() const
{
	return CryIcon("icons:Assets/Character.ico");
}

CAsset* CCharacterDefinitionType::Import(const string&, CAssetImportContext& context) const
{
	const string absOutputDirectoryPath = PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), context.GetOutputDirectoryPath());
	const string outputFilePath = PathUtil::Make(absOutputDirectoryPath, PathUtil::ReplaceExtension(PathUtil::GetFile(context.GetInputFilePath()), "cdf"));

	string skeletonFilePath;
	string skinFilePath;
	string materialFilePath;

	CAsset* pSkeletonAsset = nullptr;

	// Check what other asset types have been written so far.
	for (CAsset* const pAsset : context.GetImportedAssets())
	{
		CRY_ASSERT(pAsset && pAsset->GetType());

		if (!pAsset->GetFilesCount())
		{
			continue;
		}
		const string filePath = pAsset->GetFile(0);
		const string typeName = pAsset->GetType()->GetTypeName();
		if (typeName == "Skeleton")
		{
			skeletonFilePath = filePath;
			pSkeletonAsset = pAsset;
		}
		else if (typeName == "SkinnedMesh")
		{
			skinFilePath = filePath;
		}
		else if (typeName == "Material")
		{
			materialFilePath = filePath;
		}
	}

	// Write chrparams file, if necessary.
	if (pSkeletonAsset)
	{
		bool bWroteChrParams = false;
		const string chrParamsAssetPath = PathUtil::ReplaceExtension(skeletonFilePath, "chrparams");
		const string chrParamsAbsFilePath = PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), chrParamsAssetPath);
		const QString chrparams = MeshImporter::CreateCHPARAMS(QtUtil::ToQString(context.GetOutputDirectoryPath()));
		if (!chrparams.isEmpty())
		{
			if (WriteToFile(QtUtil::ToQString(chrParamsAbsFilePath), chrparams))
			{
				CEditableAsset editableSkeletonAsset = context.CreateEditableAsset(*pSkeletonAsset);
				editableSkeletonAsset.AddFile(chrParamsAssetPath);
				editableSkeletonAsset.WriteToFile();
				bWroteChrParams = true;
			}
		}
		if (!bWroteChrParams)
		{
			CryWarning(VALIDATOR_MODULE_ASSETS, VALIDATOR_WARNING, string().Format("Unable to write .chrparams file '%s'", PathUtil::ToUnixPath(chrParamsAbsFilePath).c_str()));
		}
	}

	const QString cdf = CreateCDF(
		QtUtil::ToQString(skeletonFilePath),
		QtUtil::ToQString(skinFilePath),
		QtUtil::ToQString(materialFilePath));
	if (cdf.isEmpty())
	{
		CryWarning(VALIDATOR_MODULE_ASSETS, VALIDATOR_WARNING, "Skip empty character definition");
		return nullptr;
	}

	if (!WriteToFile(QtUtil::ToQString(outputFilePath), cdf))
	{
		CryWarning(VALIDATOR_MODULE_ASSETS, VALIDATOR_ERROR, string().Format("Writing character definition file '%s' failed."),
			PathUtil::ToUnixPath(outputFilePath).c_str());
		return nullptr;
	}

	// Write cryasset.
	{
		CRcCaller rcCaller;
		rcCaller.OverwriteExtension("cryasset");
		rcCaller.SetAdditionalOptions("/assettypes=cgf,Mesh;chr,Skeleton;skin,SkinnedMesh;dds,Texture;mtl,Material;cga,AnimatedMesh;anm,MeshAnimation;cdf,Character;caf,Animation;wav,Sound;ogg,Sound;cax,GeometryCache;lua,Script;pfx,Particles");
		if (!rcCaller.Call(outputFilePath))
		{
			CryWarning(VALIDATOR_MODULE_ASSETS, VALIDATOR_ERROR, string().Format("Compiling file '%s' failed.",
				PathUtil::ToUnixPath(outputFilePath).c_str()));
			return nullptr;
		}
	}

	return context.LoadAsset(PathUtil::ReplaceExtension(PathUtil::ToGamePath(outputFilePath), "cdf.cryasset"));
}

std::vector<string> CCharacterDefinitionType::GetImportExtensions() const
{
	return { "fbx" };
}

std::vector<string> CCharacterDefinitionType::GetImportDependencyTypeNames() const
{
	return{ "Mesh", "SkinnedMesh", "AnimatedMesh", "Material" };
}
