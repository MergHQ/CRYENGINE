// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AssetTypeHelpers.h"
#include "ImporterUtil.h"
#include "GenerateCharacter.h"
#include "RcCaller.h"

#include <AssetSystem/Asset.h>
#include <AssetSystem/AssetType.h>
#include <AssetSystem/AssetImportContext.h>
#include <FilePathUtil.h>

#include <QDir>
#include <QTemporaryFile>

QVariantMap GetSubMaterialMapping(const FbxTool::CScene* pFbxScene)
{
	QVariantMap map;
	for (int i = 0, N = pFbxScene->GetMaterialCount(); i < N; ++i)
	{
		const FbxTool::SMaterial* const pMaterial = pFbxScene->GetMaterialByIndex(i);
		map[pMaterial->szName] = pFbxScene->GetMaterialSubMaterialIndex(pMaterial);
	}
	return map;
}

void ApplySubMaterialMapping(FbxMetaData::SMetaData& metaData, const QVariantMap& map)
{
	metaData.materialData.clear();
	metaData.materialData.reserve(map.size());
	bool ok = true;
	for (auto it = map.begin(); it != map.end(); ++it)
	{
		FbxMetaData::SMaterialMetaData mmd;
		mmd.name = QtUtil::ToString(it.key());
		mmd.settings.subMaterialIndex = it.value().toInt(&ok);
		CRY_ASSERT(ok && "Conversion from QVariant to int failed");
		metaData.materialData.push_back(mmd);
	}
}

string GetMaterialName(const CAssetImportContext& context)
{
	const CAsset* pMaterial = nullptr;
	const std::vector<CAsset*>& assets = context.GetImportedAssets();
	for(const CAsset* pAsset : assets)
	{
		if (!strcmp("Material", pAsset->GetType()->GetTypeName()))
		{
			pMaterial = pAsset;
			break;
		}
	}
	if (pMaterial)
	{
		for (int i = 0, N = pMaterial->GetFilesCount(); i < N; ++i)
		{
			if (!stricmp(PathUtil::GetExt(pMaterial->GetFile(i)), "mtl"))
			{
				return PathUtil::RemoveExtension(pMaterial->GetFile(i));
			}
		}
	}
	return string();
}

string GetFormattedAxes(CAssetImportContext& context)
{
	ICharacterGenerator* const pCharGen = CreateCharacterGenerator(context);
	CRY_ASSERT(pCharGen);
	FbxTool::CScene* const pScene = pCharGen->GetScene();
	CRY_ASSERT(pScene);
	return GetFormattedAxes(pScene->GetUpAxis(), pScene->GetForwardAxis());
}

string ImportAsset(
	const string& inputFilePath,
	const string& outputDirectoryPath,
	const std::function<void(FbxMetaData::SMetaData&)>& initMetaData)
{	
	const string absOutputDirectoryPath = PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), outputDirectoryPath);

	if (inputFilePath.empty() || absOutputDirectoryPath.empty())
	{
		return nullptr;
	}

	CFileImporter fileImporter;
	string outputFilePath = PathUtil::Make(absOutputDirectoryPath, PathUtil::GetFile(inputFilePath));
	if (!fileImporter.Import(inputFilePath, outputFilePath))
	{
		CryWarning(VALIDATOR_MODULE_ASSETS, VALIDATOR_ERROR, fileImporter.GetError().c_str());
		return string();
	}

	FbxMetaData::SMetaData metaData;
	metaData.sourceFilename = PathUtil::GetFile(inputFilePath).c_str();
	initMetaData(metaData);

	string outputFilename = PathUtil::GetFileName(inputFilePath.c_str());
	auto pTempFile = WriteTemporaryFile(QtUtil::ToQString(absOutputDirectoryPath), metaData.ToJson(), QtUtil::ToQString(outputFilename));
	if (!pTempFile)
	{
		return string();
	}

	// Write uncompiled file.
	{
		CRcCaller rcCaller;
		rcCaller.OverwriteExtension("fbx");
		rcCaller.SetAdditionalOptions("/assettypes=cgf,Mesh;chr,Skeleton;skin,SkinnedMesh;dds,Texture;mtl,Material;cga,AnimatedMesh;anm,MeshAnimation;cdf,Character;caf,Animation;wav,Sound;ogg,Sound;cax,GeometryCache;lua,Script;pfx,Particles");
		if (!rcCaller.Call(QtUtil::ToString(pTempFile->fileName())))
		{
			CryWarning(VALIDATOR_MODULE_ASSETS, VALIDATOR_ERROR, string().Format("Importing %s from '%s' failed.",
				metaData.outputFileExt.c_str(),
				PathUtil::ToUnixPath(inputFilePath).c_str()));
			return string();
		}
	}

	// Compile.
	/*
	{
		CRcCaller rcCaller;
		rcCaller.SetAdditionalOptions("/refresh");
		const string filePath = PathUtil::ReplaceExtension(outputFilePath, metaData.outputFileExt);
		if (!rcCaller.Call(filePath))
		{
			CryWarning(VALIDATOR_MODULE_ASSETS, VALIDATOR_ERROR, string().Format("Compiling %s from '%s' failed.",
				metaData.outputFileExt.c_str(),
				PathUtil::ToUnixPath(inputFilePath).c_str()));
			return string();
		}
	}
	*/

	// Note that "ReplaceExtension" does not work here.
	return PathUtil::Make(outputDirectoryPath, outputFilename + "." + metaData.outputFileExt + ".cryasset");
}

ICharacterGenerator* CreateCharacterGenerator(CAssetImportContext& context)
{
	const string key = "CharacterGenerator";
	if (!context.HasSharedPointer(key))
	{
		// At this point we can safely assume that the FBX has already been copied to the output directory.
		const string sourceFilename = PathUtil::GetFile(context.GetInputFilePath());
		const string inputFilePath = PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), PathUtil::Make(context.GetOutputDirectoryPath(), sourceFilename));

		auto pCharGen = CreateCharacterGenerator(inputFilePath).release();
		context.SetSharedPointer(key, std::shared_ptr<ICharacterGenerator>(pCharGen));
		return pCharGen;
	}
	return (ICharacterGenerator*)context.GetSharedPointer(key);
}
