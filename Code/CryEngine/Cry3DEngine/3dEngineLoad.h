// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

//! Helper class for time-sliced level loading state
class C3DEngineLevelLoadTimeslicer
{
public:
#define PRELOAD_OBJECTS_SLICED (1)

	enum class EStep
	{
		Init,
		LoadConfiguration,
		LoadUserShaders,
		LoadDefaultAssets,
		LoadMaterials,
		PreloadMergedMeshes,
#if PRELOAD_OBJECTS_SLICED
		StartPreloadLevelObjects,
		UpdatePreloadLevelObjects,
#else
		PreloadLevelObjects,
#endif
		PreloadLevelCharacters,
		LoadTerrain,
		LoadVisAreas,
		SyncMergedMeshesPreparation,
		LoadParticles,
		LoadMissionData,
		LoadSvoTiSettings,

		// Results
		Done,
		Failed,
		Count
	};

	C3DEngineLevelLoadTimeslicer(C3DEngine& owner, const char* szFolderName, XmlNodeRef&& missionXml);
	~C3DEngineLevelLoadTimeslicer();

	I3DEngine::ELevelLoadStatus DoStep();

private:

	bool ShouldPreloadLevelObjects() const;

	I3DEngine::ELevelLoadStatus SetInProgress(EStep nextStep)
	{
		m_currentStep = nextStep;
		return I3DEngine::ELevelLoadStatus::InProgress;
	}

	I3DEngine::ELevelLoadStatus SetDone()
	{
		m_currentStep = EStep::Done;
		return I3DEngine::ELevelLoadStatus::Done;
	}

	I3DEngine::ELevelLoadStatus SetFailed()
	{
		m_currentStep = EStep::Failed;
		return I3DEngine::ELevelLoadStatus::Failed;
	}


	// Inputs
	C3DEngine& m_owner;
	string m_folderName;
	XmlNodeRef m_missionXml;

	// Intermediate
	XmlNodeRef m_xmlLevelData;
	stl::scoped_set<bool> m_setInLoad;

	// state support
	EStep m_currentStep = EStep::Init;
};
