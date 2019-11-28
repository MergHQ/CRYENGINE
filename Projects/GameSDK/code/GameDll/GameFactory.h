// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

/*************************************************************************
  Description: Register the factory templates used to create classes from names
               e.g. REGISTER_FACTORY(pFramework, "Player", CPlayer, false);
               or   REGISTER_FACTORY(pFramework, "Player", CPlayerG4, false);

               Since overriding this function creates template based linker errors,
               it's been replaced by a standalone function in its own cpp file.
*************************************************************************/

struct IGameFramework;

// Register the factory templates used to create classes from names. Called via CGame::Init()
void InitGameFactory(IGameFramework *pFramework);
