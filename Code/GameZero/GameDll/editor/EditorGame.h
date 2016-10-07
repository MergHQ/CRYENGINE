// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryGame/IGameRef.h>
#include <CrySandbox/IEditorGame.h>

struct IGameStartup;

class CEditorGame : public IEditorGame
{
public:
	CEditorGame() {}
	virtual ~CEditorGame() {}

	// IEditorGame
	virtual bool                       Init(ISystem* pSystem, IGameToEditorInterface* pGameToEditorInterface) override { return true; }
	virtual void                       Update(bool haveFocus, unsigned int updateFlags) override {}
	virtual void                       Shutdown() override;
	virtual void                       SetGameMode(bool bGameMode) override {}
	virtual bool                       SetPlayerPosAng(Vec3 pos, Vec3 viewDir) override { return false; }
	virtual void                       OnBeforeLevelLoad() override {}
	virtual void                       OnAfterLevelLoad(const char* levelName, const char* levelFolder) override {}
	virtual void                       OnCloseLevel() override                                  {}
	virtual void                       OnSaveLevel() override                                   {}
	virtual bool                       BuildEntitySerializationList(XmlNodeRef output) override { return true; }
	virtual bool                       GetAdditionalMinimapData(XmlNodeRef output) override     { return false; }
	virtual IEquipmentSystemInterface* GetIEquipmentSystemInterface() override                                                   { return nullptr; }
	virtual void                       RegisterTelemetryTimelineRenderers(Telemetry::ITelemetryRepository* pRepository) override {}
	virtual void                       OnDisplayRenderUpdated(bool displayHelpers) override                                      {}
	virtual void                       OnAfterLevelInit(const char* levelName, const char* levelFolder) override                 {}
	virtual void                       OnEntitySelectionChanged(EntityId entityId, bool isSelected) override                     {}
	virtual void                       OnReloadScripts(EReloadScriptsType scriptsType) override                                  {}
	// ~IEditorGame
};
