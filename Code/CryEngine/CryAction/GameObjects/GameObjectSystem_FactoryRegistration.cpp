// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GameObjectSystem.h"

#include "WorldQuery.h"
#include "Interactor.h"
#include "GameVolumes/GameVolume_Water.h"
#include "GameObjects/MannequinObject.h"

#include <CryGame/IGameVolumes.h>
#include <CryGame/IGameFramework.h>

// entityClassString can be different from extensionClassName to allow mapping entity classes to differently
// named C++ Classes
#define REGISTER_GAME_OBJECT_EXTENSION(framework, entityClassString, extensionClassName, script)  \
  {                                                                                               \
    IEntityClassRegistry::SEntityClassDesc clsDesc;                                               \
    clsDesc.sName = entityClassString;                                                            \
    clsDesc.sScriptFile = script;                                                                 \
    struct C ## extensionClassName ## Creator : public IGameObjectExtensionCreatorBase            \
    {                                                                                             \
      IGameObjectExtension* Create(IEntity *pEntity)                                            \
      {                                                                                           \
        return pEntity->CreateComponentClass<C ## extensionClassName>();                          \
      }                                                                                           \
      void GetGameObjectExtensionRMIData(void** ppRMI, size_t * nCount)                           \
      {                                                                                           \
        C ## extensionClassName::GetGameObjectExtensionRMIData(ppRMI, nCount);                    \
      }                                                                                           \
    };                                                                                            \
    static C ## extensionClassName ## Creator _creator;                                           \
    framework->GetIGameObjectSystem()->RegisterExtension(entityClassString, &_creator, &clsDesc); \
  }                                                                                               \

#define HIDE_FROM_EDITOR(className)                                                           \
  { IEntityClass* pItemClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(className); \
    pItemClass->SetFlags(pItemClass->GetFlags() | ECLF_INVISIBLE); }

#define REGISTER_EDITOR_VOLUME_CLASS(frameWork, className)                                         \
  {                                                                                                \
    IGameVolumes* pGameVolumes = frameWork->GetIGameVolumesManager();                              \
    IGameVolumesEdit* pGameVolumesEdit = pGameVolumes ? pGameVolumes->GetEditorInterface() : NULL; \
    if (pGameVolumesEdit != NULL)                                                                  \
    {                                                                                              \
      pGameVolumesEdit->RegisterEntityClass(className);                                            \
    }                                                                                              \
  }

void CGameObjectSystem::RegisterFactories(IGameFramework* pFrameWork)
{
	REGISTER_FACTORY(pFrameWork, "WorldQuery", CWorldQuery, false);
	REGISTER_FACTORY(pFrameWork, "Interactor", CInteractor, false);

	REGISTER_GAME_OBJECT_EXTENSION(pFrameWork, "WaterVolume", GameVolume_Water, "Scripts/Entities/Environment/WaterVolume.lua");
	REGISTER_GAME_OBJECT_EXTENSION(pFrameWork, "MannequinObject", MannequinObject, "Scripts/Entities/Anim/MannequinObject.lua");
	HIDE_FROM_EDITOR("WaterVolume");
	REGISTER_EDITOR_VOLUME_CLASS(pFrameWork, "WaterVolume");
}
