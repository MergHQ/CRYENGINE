// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "FbxScene.h"
#include "FbxMetaData.h"
#include <yasli/Archive.h>

class CMaterial;

class CGlobalImportSettings
{
private:
	struct SGeneralSettings
	{
		string              m_inputFilePath;
		string              m_outputFilePath;

		string              m_inputFilename;
		string              m_outputFilename;
		SGeneralSettings();

		void Serialize(yasli::Archive& ar);
	};

	struct SStaticMeshSettings
	{
		bool                m_bMergeAllNodes;
		bool                m_bSceneOrigin;

		SStaticMeshSettings();

		void Serialize(yasli::Archive& ar);
	};

	struct SConversionSettings
	{
		// Displayed in property tree.
		FbxMetaData::Units::EUnitSetting m_unit;
		float                            m_scale;
		FbxTool::Axes::EAxis             m_forward;
		FbxTool::Axes::EAxis             m_up;

		QString                          m_fileUnitSizeInCm;

		// By comparing forward and up to their old values, we can figure
		// out which one changed most recently.
		FbxTool::Axes::EAxis m_oldForward;
		FbxTool::Axes::EAxis m_oldUp;

		SConversionSettings();

		void Serialize(yasli::Archive& ar);

		void SetForwardAxis(FbxTool::Axes::EAxis axis);
		void SetUpAxis(FbxTool::Axes::EAxis axis);
	};
public:
	CGlobalImportSettings();
	~CGlobalImportSettings();

	FbxMetaData::Units::EUnitSetting GetUnit() const;
	float                            GetScale() const;
	FbxTool::Axes::EAxis             GetUpAxis();
	FbxTool::Axes::EAxis             GetForwardAxis();
	bool                             IsMergeAllNodes() const;
	bool                             IsSceneOrigin() const;

	void                             SetInputFilePath(const string& path);
	void                             SetOutputFilePath(const string& path);
	void                             ClearFilePaths();

	void                             SetFileUnitSizeInCm(double scale);
	void                             SetUnit(FbxMetaData::Units::EUnitSetting unit);
	void                             SetScale(float scale);
	void                             SetForwardAxis(FbxTool::Axes::EAxis axis);
	void                             SetUpAxis(FbxTool::Axes::EAxis axis);
	void                             SetMergeAllNodes(bool bMergeAllNodes);
	void                             SetSceneOrigin(bool bSceneOrigin);

	void SetStaticMeshSettingsEnabled(bool bEnabled);

	void                             Serialize(yasli::Archive& ar);
private:
	SGeneralSettings m_generalSettings;
	SStaticMeshSettings m_staticMeshSettings;
	SConversionSettings m_conversionSettings;
	bool m_bStaticMeshSettingsEnabled;
};
