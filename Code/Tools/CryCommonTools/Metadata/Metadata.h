// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "CryExtension\CryGUID.h"

class IXmlNode;

namespace AssetManager
{

//! The metadata structure intended to be one for all asset types.
struct SAssetMetadata
{
	enum EVersion { eVersion_Actual = 0 };

	int32               version = eVersion_Actual; //!< For data format control and conversion.
	string              type;                      //!< Class name of the type, resolved to class desc pointer at runtime.
	CryGUID             guid = CryGUID::Null();
	std::vector<string> files;
	string              source;
	std::vector<std::pair<string, string>> details;
};

//! Returns true if successful, otherwise returns false.
bool WriteMetadata(XmlNodeRef& asset, const SAssetMetadata& metadata);

//! Returns true if successful, otherwise returns false.
bool ReadMetadata(XmlNodeRef& asset, SAssetMetadata& metadata);

//! Returns true if successful or metadata does not exist, otherwise returns false.
bool DeleteMetadata(XmlNodeRef& asset);

}
