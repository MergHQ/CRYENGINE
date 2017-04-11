// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "GlobalImportSettings.h"

#include <Cry3DEngine/I3DEngine.h>
#include <IEditor.h>
#include <IResourceSelectorHost.h>
#include <Material\Material.h>
#include <Material\MaterialManager.h>

#include <yasli/Archive.h>
#include <yasli/Enum.h>
#include <CrySerialization/StringList.h>

YASLI_ENUM_BEGIN_NESTED2(FbxTool, Axes, EAxis, "Axis")
YASLI_ENUM(FbxTool::Axes::EAxis::NegX, "-X", "-X");
YASLI_ENUM(FbxTool::Axes::EAxis::NegY, "-Y", "-Y");
YASLI_ENUM(FbxTool::Axes::EAxis::NegZ, "-Z", "-Z");
YASLI_ENUM(FbxTool::Axes::EAxis::PosX, "+X", "+X");
YASLI_ENUM(FbxTool::Axes::EAxis::PosY, "+Y", "+Y");
YASLI_ENUM(FbxTool::Axes::EAxis::PosZ, "+Z", "+Z");
YASLI_ENUM_END()

// If 'scale' corresponds to one of the known standard units (with some small
// tolerance), the unit's symbol is returned. Otherwise, the value of 'scale'
// is converted to string.
static QString FormatUnitSizeInCm(double scale)
{
	for (int i = 0; i < FbxMetaData::Units::eUnitSetting_COUNT; ++i)
	{
		const auto unit = (FbxMetaData::Units::EUnitSetting)i;
		if (FbxMetaData::Units::eUnitSetting_FromFile != unit)
		{
			if (IsEquivalent((float)scale, (float)FbxMetaData::Units::GetUnitSizeInCm(unit, -1.0)))
			{
				return QString::fromLatin1(FbxMetaData::Units::GetSymbol(unit));
			}
		}
	}
	return QString::fromLatin1("%1").arg(scale);
}

static FbxTool::Axes::EAxis GetNextAxisWrapped(FbxTool::Axes::EAxis axis)
{
	return FbxTool::Axes::EAxis(((int)axis + 1) % (int)FbxTool::Axes::EAxis::COUNT);
}

////////////////////////////////////////////////////

CGlobalImportSettings::SConversionSettings::SConversionSettings()
	: m_unit(FbxMetaData::Units::EUnitSetting::eUnitSetting_FromFile)
	, m_scale(1.0f)
	, m_forward(FbxTool::Axes::EAxis::PosX)
	, m_up(FbxTool::Axes::EAxis::PosY)
	, m_oldForward(FbxTool::Axes::EAxis::PosX)
	, m_oldUp(FbxTool::Axes::EAxis::PosY)
{}

void CGlobalImportSettings::SConversionSettings::Serialize(yasli::Archive& ar)
{
	Serialization::StringList options;
	FbxMetaData::Units::ForallNames([&options](const char* szUnitName)
	{
		options.push_back(szUnitName);
	});
	options[0] += string(" (") + QtUtil::ToString(m_fileUnitSizeInCm) + string(")");

	Serialization::StringListValue dropDown(options, (int)m_unit);
	ar(dropDown, "unit", "Unit");

	if (ar.isInput())
	{
		m_unit = (FbxMetaData::Units::EUnitSetting)dropDown.index();
	}

	ar(m_scale, "scale", "Scale");
	m_scale = std::max(0.0f, m_scale);

	ar(m_forward, "forward", "Forward");
	ar(m_up, "up", "Up");

	// Make axes selection consistent.

	if (m_forward != m_oldForward)
	{
		assert(!ar.isEdit() || m_up == m_oldUp);
		SetForwardAxis(m_forward);
	}
	else if (m_up != m_oldUp)
	{
		assert(!ar.isEdit() || m_forward == m_oldForward);
		SetUpAxis(m_up);
	}
}

void CGlobalImportSettings::SConversionSettings::SetForwardAxis(FbxTool::Axes::EAxis axis)
{
	m_oldForward = m_forward = axis;
	while (FbxTool::Axes::Abs(m_forward) == FbxTool::Axes::Abs(m_up))
	{
		m_up = GetNextAxisWrapped(m_up);
	}
}

void CGlobalImportSettings::SConversionSettings::SetUpAxis(FbxTool::Axes::EAxis axis)
{
	m_oldUp = m_up = axis;
	while (FbxTool::Axes::Abs(m_forward) == FbxTool::Axes::Abs(m_up))
	{
		m_forward = GetNextAxisWrapped(m_forward);
	}
}

////////////////////////////////////////////////////

CGlobalImportSettings::SGeneralSettings::SGeneralSettings()
{}

CGlobalImportSettings::SStaticMeshSettings::SStaticMeshSettings()
	: m_bMergeAllNodes(false)
	, m_bSceneOrigin(false)
{}

CGlobalImportSettings::CGlobalImportSettings()
	: m_bStaticMeshSettingsEnabled(true)
{}

CGlobalImportSettings::~CGlobalImportSettings() {}

FbxMetaData::Units::EUnitSetting CGlobalImportSettings::GetUnit() const
{
	return m_conversionSettings.m_unit;
}

float CGlobalImportSettings::GetScale() const
{
	return m_conversionSettings.m_scale;
}

FbxTool::Axes::EAxis CGlobalImportSettings::GetUpAxis()
{
	return m_conversionSettings.m_up;
}

FbxTool::Axes::EAxis CGlobalImportSettings::GetForwardAxis()
{
	return m_conversionSettings.m_forward;
}

bool CGlobalImportSettings::IsMergeAllNodes() const
{
	return m_staticMeshSettings.m_bMergeAllNodes;
}

bool CGlobalImportSettings::IsSceneOrigin() const
{
	return m_staticMeshSettings.m_bSceneOrigin;
}

void CGlobalImportSettings::SetInputFilePath(const string& filePath)
{
	m_generalSettings.m_inputFilePath = filePath;
	m_generalSettings.m_inputFilename = PathUtil::GetFileName(m_generalSettings.m_inputFilePath);
}

void CGlobalImportSettings::SetOutputFilePath(const string& filePath)
{
	m_generalSettings.m_outputFilePath = filePath;
	m_generalSettings.m_outputFilename = PathUtil::GetFileName(m_generalSettings.m_outputFilePath);
}

void CGlobalImportSettings::ClearFilePaths()
{
	m_generalSettings.m_inputFilePath.clear();
	m_generalSettings.m_outputFilePath.clear();
	m_generalSettings.m_inputFilename.clear();
	m_generalSettings.m_outputFilename.clear();
}

void CGlobalImportSettings::SetFileUnitSizeInCm(double scale)
{
	m_conversionSettings.m_fileUnitSizeInCm = FormatUnitSizeInCm(scale);
}

void CGlobalImportSettings::SetUnit(FbxMetaData::Units::EUnitSetting unit)
{
	m_conversionSettings.m_unit = unit;
}

void CGlobalImportSettings::SetScale(float scale)
{
	m_conversionSettings.m_scale = std::max(0.0f, scale);
}

void CGlobalImportSettings::SetForwardAxis(FbxTool::Axes::EAxis axis)
{
	m_conversionSettings.SetForwardAxis(axis);
}

void CGlobalImportSettings::SetUpAxis(FbxTool::Axes::EAxis axis)
{
	m_conversionSettings.SetUpAxis(axis);
}

void CGlobalImportSettings::SetMergeAllNodes(bool bMergeAllNodes)
{
	m_staticMeshSettings.m_bMergeAllNodes = bMergeAllNodes;
}

void CGlobalImportSettings::SetSceneOrigin(bool bSceneOrigin)
{
	m_staticMeshSettings.m_bSceneOrigin = bSceneOrigin;
}

void CGlobalImportSettings::SGeneralSettings::Serialize(yasli::Archive& ar)
{
	ar(m_inputFilename, "input_filename", "!Input filename");
	ar.doc(m_inputFilePath);

	ar(m_outputFilename, "output_filename", "!Output filename");
	ar.doc(m_outputFilePath);
}

void CGlobalImportSettings::SStaticMeshSettings::Serialize(yasli::Archive& ar)
{
	ar(m_bMergeAllNodes, "merge_all_nodes", "Merge all nodes");
	ar(m_bSceneOrigin, "scene_origin", "Scene origin");
}

void CGlobalImportSettings::Serialize(yasli::Archive& ar)
{
	ar(m_generalSettings, "general_settings", "General settings");
	if (ar.isInput() || m_bStaticMeshSettingsEnabled)
	{
		ar(m_staticMeshSettings, "static_mesh_settings", "Static mesh settings");
	}
	ar(m_conversionSettings, "conversion_settings", "Conversion settings");
}

void CGlobalImportSettings::SetStaticMeshSettingsEnabled(bool bEnabled)
{
	m_bStaticMeshSettingsEnabled = bEnabled;
}
