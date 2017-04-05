// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "FbxScene.h"
#include "FbxMetaData.h"
#include "NodeProperties.h"

#include <CryString/CryString.h>
#include <CrySerialization/IArchive.h>
#include <yasli/Archive.h>
#include <yasli/STL.h>
#include <yasli/Enum.h>
#include <yasli/JSONIArchive.h>
#include <yasli/JSONOArchive.h>

namespace FbxTool
{
namespace Meta
{

struct SNodeMeta
{
	std::vector<string>         path;
	FbxTool::ENodeExportSetting exportSetting;
	int                         lod;
	bool                        bIsProxy;
	std::vector<string>         customProperties;
	CNodeProperties             physicalProperties;

	void                        Serialize(Serialization::IArchive& ar);
};

void WriteNodeMetaData(const CScene& scene, std::vector<SNodeMeta>& nodeMeta);
void ReadNodeMetaData(const std::vector<SNodeMeta>& nodeMeta, CScene& scene);

struct SMaterialMeta
{
	string                      name;
	EMaterialPhysicalizeSetting physicalizeSetting;
	int                         subMaterialIndex;
	bool                        bMarkedForIndexAutoAssignment;
	ColorF                      color;

	void                        Serialize(Serialization::IArchive& ar);
};

void WriteMaterialMetaData(const CScene& scene, std::vector<SMaterialMeta>& materialMeta);
void ReadMaterialMetaData(const std::vector<SMaterialMeta>& materialMeta, CScene& scene);

} // namespace Meta
} // namespace FbxTool

//! Data used to restore state of editor. Not passed to RC.
struct SEditorMetaData : FbxMetaData::IEditorMetaData
{
	std::shared_ptr<CGlobalImportSettings>    pEditorGlobalImportSettings;
	std::vector<FbxTool::Meta::SNodeMeta>     editorNodeMeta;
	std::vector<FbxTool::Meta::SMaterialMeta> editorMaterialMeta;

	SEditorMetaData();

	// IEditorMetaData implementation.
	virtual void Serialize(yasli::Archive& ar) override;
};

