// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Metadata.h"
#include "IConvertor.h"
#include "CryExtension\CryGUIDHelper.h"

namespace Metadata_Private
{

static const char* GetMetadataTag() { return "AssetMetadata"; }

}

namespace AssetManager
{

bool WriteMetadata(XmlNodeRef& asset, const SAssetMetadata& metadata)
{
	using namespace Metadata_Private;

	XmlNodeRef pMetadataNode = asset->findChild(GetMetadataTag());
	if (!pMetadataNode)
	{
		pMetadataNode = asset->createNode(GetMetadataTag());
		asset->insertChild(0, pMetadataNode);
	}
	pMetadataNode->setAttr("version", metadata.version);
	pMetadataNode->setAttr("type", metadata.type);

	CRY_ASSERT(metadata.guid != CryGUID::Null());
	pMetadataNode->setAttr("guid", CryGUIDHelper::Print(metadata.guid));

	if (!metadata.source.empty())
	{
		pMetadataNode->setAttr("source", metadata.source);
	}
	else
	{
		pMetadataNode->delAttr("source");
	}

	XmlNodeRef pFiles = pMetadataNode->findChild("Files");
	if (!pFiles)
	{
		pFiles = pMetadataNode->newChild("Files");
	}
	else
	{
		pFiles->removeAllChilds();
	}

	for (const string& file : metadata.files)
	{
		XmlNodeRef pFile = pFiles->newChild("File");
		pFile->setAttr("path", file);
	}

	XmlNodeRef pDetails = pMetadataNode->findChild("Details");
	if (!pDetails)
	{
		pDetails = pMetadataNode->newChild("Details");
	}
	else
	{
		pDetails->removeAllChilds();
	}

	for (const auto& detail : metadata.details)
	{
		XmlNodeRef pDetail = pDetails->newChild("Detail");
		pDetail->setAttr("name", detail.first);
		pDetail->setContent(detail.second);
	}
	return true;
}

bool ReadMetadata(XmlNodeRef& asset, SAssetMetadata& metadata)
{
	using namespace Metadata_Private;

	XmlNodeRef pMetadataNode = asset->findChild(GetMetadataTag());

	if (!pMetadataNode || !pMetadataNode->getAttr("version", metadata.version) || (metadata.version != SAssetMetadata::eVersion_Actual))
	{
		return false;
	}
	metadata.type = pMetadataNode->getAttr("type");
	metadata.source = pMetadataNode->getAttr("source");
	if (pMetadataNode->haveAttr("guid"))
	{
		const string guid = pMetadataNode->getAttr("guid");
		metadata.guid = CryGUIDHelper::FromString(guid);
	}

	XmlNodeRef pFiles = pMetadataNode->findChild("Files");
	if (pFiles)
	{
		metadata.files.reserve(pFiles->getChildCount());
		for (int i = 0, n = pFiles->getChildCount(); i < n; ++i)
		{
			XmlNodeRef pFile = pFiles->getChild(i);
			metadata.files.emplace_back(pFile->getAttr("path"));
		}
	}

	XmlNodeRef pDetails = pMetadataNode->findChild("Details");
	if (pDetails)
	{
		const int detailCount = pDetails->getChildCount();
		metadata.details.reserve(detailCount);
		for (int i = 0; i < detailCount; ++i)
		{
			XmlNodeRef pDetail = pDetails->getChild(i);
			const char* szDetailName;
			if (!strcmp("Detail", pDetail->getTag()) && pDetail->getAttr("name", &szDetailName))
			{
				metadata.details.emplace_back(szDetailName, pDetail->getContent());
			}
		}
	}

	return true;
}

bool DeleteMetadata(XmlNodeRef& asset)
{
	using namespace Metadata_Private;

	asset->removeChild(asset->findChild(GetMetadataTag()));
	return true;
}

}
