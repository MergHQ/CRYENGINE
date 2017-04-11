// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MetadataHelpers.h"
#include "IConfig.h"
#include "IResCompiler.h"
#include "StringHelpers.h"
#include "FileUtil.h"
#include "CryGUIDHelper.h"
#include "IRCLog.h"
#include "CryExtension\CryGUIDHelper.h"

namespace AssetHelpers_Private
{

class CDictionary
{
public:
	CDictionary(string keyValuePairs)
	{
		std::vector<string> pairs;
		std::vector<string> type;
		StringHelpers::Split(keyValuePairs, ";", false, pairs);
		types.reserve(pairs.size());
		for (const string& pair : pairs)
		{
			type.resize(0);
			StringHelpers::Split(pair, ",", false, type);
			if (type.size() == 2)
			{
				types.emplace_back(type[0], type[1]);
			}
		}

		std::sort(types.begin(), types.end(), [](const auto& a, const auto& b)
		{
			return stricmp(a.first.c_str(), b.first.c_str()) < 0;
		});
	}

	const char* GetValue(const char* szKey, const char* szNotFoundValue) const
	{
		auto it = std::lower_bound(types.begin(), types.end(), szKey, [](const auto& a, const auto& b)
		{
			return stricmp(a.first.c_str(), b) < 0;
		});

		return ((it != types.end()) && (stricmp(szKey, (*it).first) == 0)) ? (*it).second.c_str() : szNotFoundValue;
	}

private:
	std::vector<std::pair<string, string>> types;
};

static string GetMetadataType(const char* szExt, const IConfig* pConfig)
{
	const AssetHelpers_Private::CDictionary assetTypes(pConfig->GetAsString("AssetTypes", "", ""));
	return assetTypes.GetValue(szExt, szExt);
}

static string GetAssetFilepath(const string& sourceFilepath)
{
	using namespace AssetManager;

	//  Special handling of RC intermediate files: filename.$ext -> filename.$ext.$cryasset
	const char* szExt = PathUtil::GetExt(sourceFilepath);
	if (szExt && szExt[0] == '$')
	{
		return  sourceFilepath + ".$" + GetAssetExtension();
	}

	return  sourceFilepath + "." + GetAssetExtension();
}

static void UpdateDetails(AssetManager::SAssetMetadata& metadata, const std::vector<std::pair<string, string>>& details)
{
	for (const auto& item : details)
	{
		auto existingData = std::find_if(metadata.details.begin(), metadata.details.end(), [&item](const std::pair<string, string>& x)
		{
			return x.first.CompareNoCase(item.first) == 0;
		});

		if (existingData != metadata.details.end())
		{
			(*existingData).second = item.second;
		}
		else
		{
			metadata.details.push_back(item);
		}
	}
}

}

namespace AssetManager
{

const char* GetAssetExtension() { return "cryasset"; }

void InitMetadata(AssetManager::SAssetMetadata& metadata, const IConfig* pConfig, const string& sourceFilepath, const std::vector<string>& files)
{
	using namespace AssetHelpers_Private;

	assert(files.size());

	const AssetHelpers_Private::CDictionary userValues(pConfig->GetAsString("cryasset", "", ""));

	if (metadata.guid == CryGUID::Null())
	{
		metadata.guid = CryGUIDHelper::Create();
	}

	const char* szExt = PathUtil::GetExt(files.begin()->c_str());
	metadata.type = GetMetadataType(szExt, pConfig);
	metadata.files.clear();
	metadata.files.reserve(files.size());
	for (const string& path : files)
	{
		metadata.files.emplace_back(PathUtil::GetFile(path));
	}

	const char* szSource = userValues.GetValue("source", nullptr);
	if (szSource)
	{
		metadata.source = szSource;
	}
	else if (!sourceFilepath.empty() && (sourceFilepath != files[0]))
	{
		metadata.source = PathUtil::GetFile(sourceFilepath);
	}

	UpdateDetails(metadata, CollectMetadataDetails(files.begin()->c_str()));
}

bool SaveAsset(IResourceCompiler* pRc, const IConfig* pConfig, const string& sourceFilepath, const std::vector<string>& assetFilepaths)
{
	if (pConfig->GetAsBool("stripMetadata", false, true))
	{
		return true;
	}

	assert(assetFilepaths.size());

	using namespace AssetHelpers_Private;

	SAssetMetadata metadata;

	const string asset = GetAssetFilepath(assetFilepaths.front());
	const string parentAsset = GetAssetFilepath(sourceFilepath);

	auto xml = FileUtil::FileExists(asset) ? pRc->LoadXml(asset) : pRc->CreateXml(GetAssetExtension());
	AssetManager::ReadMetadata(xml, metadata);

	InitMetadata(metadata, pConfig, sourceFilepath, assetFilepaths);

	// Keep in metadata of intermediate steps for multi-step import.
	if (FileUtil::FileExists(parentAsset))
	{
		SAssetMetadata parentMetadata;
		auto ancestorXml = pRc->LoadXml(parentAsset);
		AssetManager::ReadMetadata(ancestorXml, parentMetadata);

		if (!parentMetadata.source.empty())
		{
			// We want to keep the original source for a chain of transformations like ".tif" -> .$dds -> ... -> ".dds". 
			metadata.source = parentMetadata.source;
		}

		// Maintain others necessary fields (if any) here.
	}

	if (!WriteMetadata(xml, metadata) || !xml->saveToFile(asset))
	{
		RCLogError("Can't write metadata file '%s'", asset.c_str());
		return false;
	}

	pRc->AddInputOutputFilePair(sourceFilepath, asset);
	// TODO: Consider moving here registration of AddInputOutputFilePair for all asset files.

	return true;
}

}
