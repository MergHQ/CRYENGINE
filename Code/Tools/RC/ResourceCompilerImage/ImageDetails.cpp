// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "ImageDetails.h"
#include "Formats/DDS.h"
#include "Formats/TIFF.h"
#include "Metadata/Metadata.h"
#include "ImageObject.h"
#include <CrySystem/XML/IXml.h>
#include <FileUtil.h>

namespace AssetManager
{

// Collects metadata for Sandbox asset system (*.cryasset).

bool CollectDDSImageDetails(XmlNodeRef& xmlnode, const char* szFilename, IResourceCompiler* pRc)
{
	CImageProperties props;
	std::unique_ptr<ImageObject> image(ImageDDS::LoadByUsingDDSLoader(szFilename, &props, string()));
	if (!image)
	{
		return true;
	}

	XmlNodeRef metadata = GetMetadataNode(xmlnode);
	if (!metadata)
	{
		return false;
	}

	uint32 width;
	uint32 height;
	uint32 mipCount;
	image->GetExtent(width, height, mipCount);
	std::vector<std::pair<string, string>> details;
	details.emplace_back("width", string().Format("%u", width));
	details.emplace_back("height", string().Format("%u", height));
	details.emplace_back("mipCount", string().Format("%u", mipCount));
	AddDetails(metadata, details);
	return true;
}

string GetValueByKey(const string& specialInstructions, const string& key)
{
	std::vector<string> values;
	StringHelpers::Split(specialInstructions, " ", false, values);
	for (const string& value : values)
	{
		if (strncmp(value.c_str(), key.c_str(), key.length()) == 0)
		{
			return value.substr(key.length() + 1);
		}
	}
	return string();
}

bool CollectTifImageDetails(XmlNodeRef& xmlnode, const char* szFilename, IResourceCompiler* pRc)
{
	if (!FileUtil::FileExists(szFilename))
	{
		return true;
	}

	string specialInstructions = ImageTIFF::LoadSpecialInstructionsByUsingTIFFLoader(szFilename);
	if (specialInstructions.empty())
	{
		return true;
	}

	XmlNodeRef metadata = GetMetadataNode(xmlnode);
	if (!metadata)
	{
		return false;
	}

	string keyValuePairs = GetValueByKey(specialInstructions, string("/cryasset"));
	if (keyValuePairs.empty())
	{
		return true;
	}

	std::vector<string> pairs;
	std::vector<std::pair<string, string>> details;
	StringHelpers::Split(keyValuePairs, ";", false, pairs);
	details.reserve(pairs.size());
	std::vector<string> detail;
	for (const string& pair : pairs)
	{
		detail.clear();
		StringHelpers::Split(pair, ",", false, detail);
		if (detail.size() == 2)
		{
			details.emplace_back(detail[0], detail[1]);
		}
	}
	AddDetails(metadata, details);
	return true;
}

}
