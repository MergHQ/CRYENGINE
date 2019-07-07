// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IEditorImpl.h"
#include "DocMultiArchive.h"
#include <CrySystem/TimeValue.h>

class CMission;
class CLevelShaderCache;
class CAutoDocNotReady;
struct ICVar;
struct LightingSettings;
struct IVariable;

class SANDBOX_API CCryEditDoc : public _i_reference_target_t
{
	friend CAutoDocNotReady;
public:
	CCryEditDoc();
	virtual ~CCryEditDoc();

	BOOL                              DoSave(LPCTSTR lpszPathName, BOOL bReplace);//TODO : change the signature of this!
	BOOL                              OnOpenDocument(LPCTSTR lpszPathName);
	CLevelShaderCache*                GetShaderCache()           { return m_pLevelShaderCache; }
	LightingSettings*                 GetLighting();
	XmlNodeRef&                       GetEnvironmentTemplate()   { return m_environmentTemplate; }
	XmlNodeRef&                       GetFogTemplate()           { return m_fogTemplate; }
	bool                              IsDocumentReady() const    { return m_bDocumentReady; }
	bool                              IsLevelBeingLoaded() const { return m_bLevelBeingLoaded; }
	bool                              IsLevelExported() const;
	bool                              Save();
	static const std::vector<string>& GetLevelFilenames();
	string                            GetPathName() const { return m_pathName; }
	string                            GetTitle() const;
	void                              ChangeMission();
	void                              DeleteContents();
	void                              GetMemoryUsage(ICrySizer* pSizer);
	void                              InitEmptyLevel(int resolution = 0, float unitSize = 0, bool bUseTerrain = false);
	void                              OnEnvironmentPropertyChanged(IVariable* pVar);
	void                              SaveAutoBackup(bool bForce = false);
	void                              SetDocumentReady(bool bReady);
	void                              SetLevelExported(bool boExported = true);
	void                              SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE);

	//TODO: this modified flag is only the level, not the entire data, should be moved to assets-based modified flags instead
	void SetModifiedFlag(bool bModified);

	//! Only true when closing the document
	bool IsClosing() const { return m_bIsClosing; }

	//! Get the path of the last level that was loaded last (relative to project root)
	QString GetLastLoadedLevelName();

	//! Set the relative path of the last level that was loaded last (relative to project root)
	void SetLastLoadedLevelName(const char* lastLoadedFileName);

	//! Return currently active Mission.
	CMission* GetCurrentMission(bool bSkipLoadingAIWhenSyncingContent = false);

	//! Get number of missions on Map.
	int GetMissionCount() const { return m_missions.size(); }

	//! Get Mission by index.
	CMission* GetMission(int index) const { return m_missions[index]; }

	// Check if document can be closed.
	bool CanClose();

protected:
	struct TOpenDocContext
	{
		CTimeValue loading_start_time;
		string     relativeLevelName;
	};

	struct TSaveDocContext
	{
		bool bSaved;
	};

protected:

	//Returns whether the level was modified. Not exposed anymore, please rely on asset-based modifier flags instead, should not be necessary to access from outside Sandbox
	bool         IsModified() const;
	//! Copies all files necessary for the level if the level is being saved to a new location.
	void         CopyFilesIfSavedToNewLocation(const string& levelFolder);
	//! Find Mission by name.
	CMission*    FindMission(const string& name) const;
	//! Makes specified mission current.
	void         SetCurrentMission(CMission* mission);
	//! Add new mission to map.
	void         AddMission(CMission* mission);
	//! Remove existing mission from map.
	void         RemoveMission(CMission* mission);
	//! called immediately after saving the level.
	void         ClearMissions();
	//! For saving binary data (voxel object)
	CXmlArchive* GetTmpXmlArch() { return m_pTmpXmlArchHack; }

	BOOL         AfterSaveDocument(LPCTSTR lpszPathName, TSaveDocContext& context, bool bShowPrompt = true);
	BOOL         BeforeOpenDocument(LPCTSTR lpszPathName, TOpenDocContext& context);
	BOOL         BeforeSaveDocument(LPCTSTR lpszPathName, TSaveDocContext& context);
	BOOL         DoFileSave();
	BOOL         DoOpenDocument(LPCTSTR lpszPathName, TOpenDocContext& context);
	BOOL         DoSaveDocument(LPCTSTR lpszPathName, TSaveDocContext& context);
	BOOL         LoadXmlArchiveArray(TDocMultiArchive& arrXmlAr, const string& relativeLevelName, const string& levelPath);
	BOOL         OnSaveDocument(LPCTSTR lpszPathName);
	COLORREF     GetWaterColor() { return m_waterColor; }
	bool         LoadLevel(TDocMultiArchive& arrXmlAr, const string& filename);
	bool         SaveLevel(const string& filename);
	static void  OnValidateSurfaceTypesChanged(ICVar*);
	string       GetCryIndexPath(const LPCTSTR levelFilePath);
	string       GetCurrentMissionName(TDocMultiArchive& arrXmlAr);
	void         ActivateMission(const string& currentMissionName);
	void         ForceSkyUpdate();
	void         Load(CXmlArchive& xmlAr, const string& szFilename);
	void         Load(TDocMultiArchive& arrXmlAr, const string& szFilename);
	void         LoadShaderCache(TDocMultiArchive& arrXmlAr);
	void         LogLoadTime(int time);
	void         OnStartLevelResourceList();
	void         RegisterConsoleVariables();
	void         ReleaseXmlArchiveArray(TDocMultiArchive& arrXmlAr);
	void         Save(CXmlArchive& xmlAr);
	void         Save(TDocMultiArchive& arrXmlAr);
	void         SerializeFogSettings(CXmlArchive& xmlAr);
	void         SerializeMissions(TDocMultiArchive& arrXmlAr, string& currentMission, bool bPartsInXml);
	void         SerializeShaderCache(CXmlArchive& xmlAr);
	void         SerializeViewSettings(CXmlArchive& xmlAr);
	void         SetWaterColor(COLORREF col) { m_waterColor = col; }
	void         StartStreamingLoad()        {}
	void         SyncCurrentMissionContent(bool bRetrieve);

protected:
	string                 m_strMasterCDFolder;
	string                 m_pathName;
	bool                   m_bLoadFailed;
	bool                   m_bModified;
	bool                   m_bIsClosing;
	COLORREF               m_waterColor;
	XmlNodeRef             m_fogTemplate;
	XmlNodeRef             m_environmentTemplate;
	CMission*              m_mission;
	std::vector<CMission*> m_missions;
	bool                   m_bDocumentReady;
	CXmlArchive*           m_pTmpXmlArchHack;
	CLevelShaderCache*     m_pLevelShaderCache;
	ICVar*                 doc_validate_surface_types;
	bool                   m_boLevelExported;
	bool                   m_bLevelBeingLoaded;
};

class CAutoDocNotReady
{
public:
	CAutoDocNotReady()
	{
		m_prevState = GetIEditorImpl()->GetDocument()->IsDocumentReady();
		GetIEditorImpl()->GetDocument()->SetDocumentReady(false);
	}

	~CAutoDocNotReady()
	{
		GetIEditorImpl()->GetDocument()->SetDocumentReady(m_prevState);
	}

private:
	bool m_prevState;
};
