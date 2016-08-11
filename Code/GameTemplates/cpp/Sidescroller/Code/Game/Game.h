#pragma once

#include <CryGame/IGame.h>
#include <CryGame/IGameFramework.h>
#include <../CryAction/IActionMapManager.h>

static const char* GAME_GUID = "{00000000-1111-2222-3333-444444444444}";


class CGame : public IGame
{
public:
	CGame();
	virtual ~CGame();

	// IGame
	virtual bool Init(IGameFramework *pFramework) override;
	virtual bool CompleteInit() override { return true; };
	virtual void Shutdown() override { this->~CGame(); }
	virtual int Update(bool haveFocus, unsigned int updateFlags) override;
	virtual void EditorResetGame(bool bStart) override {}
	virtual void PlayerIdSet(EntityId playerId) override {}
	virtual IGameFramework* GetIGameFramework() override { return m_pGameFramework; }
	virtual const char* GetLongName() override { return GetName(); }
	virtual const char* GetName() override;
	virtual void GetMemoryStatistics(ICrySizer* s) override { s->Add(*this); }
	virtual void OnClearPlayerIds() override {}
	virtual IGame::TSaveGameName CreateSaveGameName() override { return TSaveGameName(); }
	virtual const char* GetMappedLevelName(const char *levelName) const override { return ""; }
	virtual IGameStateRecorder* CreateGameStateRecorder(IGameplayListener* pL) override { return nullptr; }
	virtual const bool DoInitialSavegame() const override { return true; }
	virtual void LoadActionMaps(const char* filename) override {}
	virtual uint32 AddGameWarning(const char* stringId, const char* paramMessage, IGameWarningsListener* pListener = nullptr) override { return 0; }
	virtual void RenderGameWarnings() override {}
	virtual void RemoveGameWarning(const char* stringId) override {}
	virtual bool GameEndLevel(const char* stringId) override { return false; }
	virtual void OnRenderScene(const SRenderingPassInfo &passInfo) override {}
	virtual void RegisterGameFlowNodes() override;
	virtual void FullSerialize(TSerialize ser) override {}
	virtual void PostSerialize() override {}
	virtual IGame::ExportFilesInfo ExportLevelData(const char* levelName, const char* missionName) const override { return IGame::ExportFilesInfo(levelName, 0); }
	virtual void LoadExportedLevelData(const char* levelName, const char* missionName) override {}
	virtual IGamePhysicsSettings* GetIGamePhysicsSettings() override { return nullptr; }
	virtual void InitEditor(IGameToEditorInterface* pGameToEditor) override {}
	virtual void* GetGameInterface() override {return nullptr;}
	// ~IGame

protected:
	void InitializePlayerProfile();

private:
	IGameFramework* m_pGameFramework;
};
