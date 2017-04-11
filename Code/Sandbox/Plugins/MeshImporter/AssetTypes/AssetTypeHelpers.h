// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryString/CryString.h>

#include <QMap>
#include <QVariant>

namespace FbxTool
{

class CScene;

} // namespace FbxTool

namespace FbxMetaData
{

struct SMetaData;

} // namespace FbxMetaData

struct ICharacterGenerator;

class CAssetImportContext;

#define IMPORT_DATA_MATERIAL_MAPPING "materialMapping"

// Maps FBX material names to MTL sub-material IDs.
// Return value type can be converted to QVariant.
QVariantMap GetSubMaterialMapping(const FbxTool::CScene* pFbxScene);

// \param map In the format returned by GetSubMaterialMapping.
void ApplySubMaterialMapping(FbxMetaData::SMetaData& metaData, const QVariantMap& map);

//! Returns the first output file path that is a .mtl; or empty string, if no such file exists.
string GetMaterialName(const CAssetImportContext& context);

string GetFormattedAxes(CAssetImportContext& context);

string ImportAsset(
	const string& inputFilePath,
	const string& outputDirectoryPath,
	const std::function<void(FbxMetaData::SMetaData&)>& initMetaData);

//! Creates a character generator and stores it in the import context, or returns the existing instance.
//! \remark The lifetime of the character generator is managed by the context.
ICharacterGenerator* CreateCharacterGenerator(CAssetImportContext& context);