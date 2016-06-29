// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id$
   $DateTime$
   Description:

   -------------------------------------------------------------------------
   History:
   - 2:8:2004   10:53 : Created by Márcio Martins

*************************************************************************/

#pragma once

#define CRY_SAVEGAME_FILENAME "CRYENGINE"
#define CRY_SAVEGAME_FILE_EXT ".CSF"

#include <CrySystem/ICmdLine.h>
#include <CryNetwork/INetwork.h>
#include <CrySystem/ITestSystem.h> // <> required for Interfuscator

struct IAIActorProxy;
struct IGameFramework;
struct IGameStateRecorder;
struct IGameAudio;
struct IGameWarningsListener;
//struct ITestManager;
struct SGameStartParams;
struct SRenderingPassInfo;
struct IGamePhysicsSettings;
struct IGameToEditorInterface;
struct IGameWebDebugService;

//! Main interface used for the game central object.
//! The IGame interface should be implemented in the GameDLL.
//! Game systems residing in the GameDLL can be initialized and updated inside the Game object.
//! \see IEditorGame.
struct IGame
{
	struct ExportFilesInfo
	{
		ExportFilesInfo(const char* _baseFileName, const uint32 _fileCount)
			: m_pBaseFileName(_baseFileName)
			, m_fileCount(_fileCount)
		{
		}

		ILINE uint32      GetFileCount() const    { return m_fileCount; }
		ILINE const char* GetBaseFileName() const { return m_pBaseFileName; }

		static void       GetNameForFile(const char* baseFileName, const uint32 fileIdx, char* outputName, size_t outputNameSize)
		{
			assert(baseFileName != NULL);
			cry_sprintf(outputName, outputNameSize, "%s_%u", baseFileName, fileIdx);
		}

	private:
		const char*  m_pBaseFileName;
		const uint32 m_fileCount;
	};

	//! Entry function used to initialize a new IGame of a GameMod.
	//! \param pGameFramework Pointer to the game framework, from which a pointer of ISystem can be retrieved.
	//! \return Pointer to the newly created IGame implementation.
	typedef IGame*(* TEntryFunction)(IGameFramework* pGameFramework);

	//! Type to represent saved game names, keeping the string on the stack if possible.
	typedef CryStackStringT<char, 256> TSaveGameName;

	// <interfuscator:shuffle>
	virtual ~IGame(){}

	//! Initialize the MOD.
	//! The shutdown method, must be called independent of this method's return value.
	//! \param pCmdLine Pointer to the command line interface.
	//! \param pFramework Pointer to the IGameFramework interface.
	//! \return 0 if something went wrong with initialization, non-zero otherwise.
	virtual bool Init(IGameFramework* pFramework) = 0;

	//! Init editor related things.
	virtual void InitEditor(IGameToEditorInterface* pGameToEditor) = 0;

	virtual void GetMemoryStatistics(ICrySizer* s) = 0;

	//! Finish initializing the MOD.
	//! Called after the game framework has finished its CompleteInit.
	//! This is the point at which to register game flow nodes etc.
	virtual bool CompleteInit() { return true; };

	//! Shuts down the MOD and delete itself.
	virtual void Shutdown() = 0;

	//! Notify game of pre-physics update.
	virtual void PrePhysicsUpdate() {}

	//! Updates the MOD.
	//! \param haveFocus true if the game has the input focus.
	//! \return 0 to terminate the game (i.e. when quitting), non-zero to continue.
	virtual int Update(bool haveFocus, unsigned int updateFlags) = 0;

	//! Called on the game when entering/exiting game mode in editor
	//! \param bStart true if we enter game mode, false if we exit it.
	virtual void EditorResetGame(bool bStart) = 0;

	//! Called on the game when the local player id is set.
	//! \param playerId Entity Id of the local player.
	virtual void PlayerIdSet(EntityId playerId) = 0;

	//! \return Name of the mode. (e.g. "Capture The Flag").
	virtual const char* GetLongName() = 0;

	//! \return A short description of the mode. (e.g. "dc")
	virtual const char* GetName() = 0;

	//! Loads a specified action map, used mainly for loading the default action map
	virtual void LoadActionMaps(const char* filename) = 0;

	//! Called when playerID is reset in GameChannel
	virtual void OnClearPlayerIds() = 0;

	//! Auto-Savegame name creation.
	//! \return c_str or NULL.
	virtual IGame::TSaveGameName CreateSaveGameName() = 0;

	//! \return Pointer to the game framework being used.
	virtual IGameFramework* GetIGameFramework() = 0;

	//! Mapping level filename to "official" name.
	//! \return c_str or NULL.
	virtual const char* GetMappedLevelName(const char* levelName) const = 0;

	//! \return true if an initial levelstart savegame should be created, otherwise false.
	virtual const bool DoInitialSavegame() const = 0;

	//! Add a game warning that is shown to the player
	//! \return A unique handle to the warning or 0 for any error.
	virtual uint32 AddGameWarning(const char* stringId, const char* paramMessage, IGameWarningsListener* pListener = NULL) = 0;

	//! Called from 3DEngine in RenderScene, so polygons and meshes can be added to the scene from game
	virtual void OnRenderScene(const SRenderingPassInfo& passInfo) = 0;

	//! Render Game Warnings.
	virtual void RenderGameWarnings() = 0;

	//! Remove a game warning.
	virtual void RemoveGameWarning(const char* stringId) = 0;

	//! Callback to game for game specific actions on level end.
	//! \retval false, if the level end should continue.
	//! \retval true, if the game handles the end level action and calls ScheduleEndLevel directly.
	virtual bool GameEndLevel(const char* stringId) = 0;

	virtual void SetUserProfileChanged(bool yesNo) {}

	//! Creates a GameStateRecorder instance in GameDll and passes its ownership to the caller (CryAction/GamePlayRecorder).
	virtual IGameStateRecorder* CreateGameStateRecorder(IGameplayListener* pL) = 0;

	virtual void                FullSerialize(TSerialize ser) = 0;
	virtual void                PostSerialize() = 0;

	//! Editor export interface hook, to allow the game to export its own data into the level paks.
	//! \return Exported file information.
	virtual IGame::ExportFilesInfo ExportLevelData(const char* levelName, const char* missionName) const = 0;

	//! Interface hook to load all game exported data when the level is loaded.
	virtual void LoadExportedLevelData(const char* levelName, const char* missionName) = 0;

	//! Called by FlowSystem to register all game specific flow nodes.
	virtual void RegisterGameFlowNodes() = 0;

	//! Retrieve interface for the IGamePhysicsSettings.
	virtual IGamePhysicsSettings* GetIGamePhysicsSettings() = 0;

	//! Access to game interface.
	virtual void* GetGameInterface() = 0;

	//! Retrieves IGameWebDebugService for web-socket based remote debugging.
	virtual IGameWebDebugService* GetIWebDebugService() { return nullptr; };
	// </interfuscator:shuffle>
};
