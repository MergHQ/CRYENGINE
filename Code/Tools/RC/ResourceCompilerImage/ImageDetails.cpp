// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "ImageDetails.h"
#include "Formats/DDS.h"
#include "Metadata/Metadata.h"
#include "ImageObject.h"
#include <CrySystem/XML/IXml.h>

namespace AssetManager
{

// Collects metadata for Sandbox asset system (*.cryasset).

bool CollectDDSImageDetails(XmlNodeRef& xmlnode, const char* szFilename, IResourceCompiler* pRc)
{
	using namespace AssetManager;

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

}
