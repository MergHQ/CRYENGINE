// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __CET_ACTIONMAP_H__
#define __CET_ACTIONMAP_H__

#pragma once

void AddDisableActionMap(IContextEstablisher* pEst, EContextViewState state);
void AddInitActionMap_ClientActor(IContextEstablisher* pEst, EContextViewState state);
void AddInitActionMap_OriginalSpectator(IContextEstablisher* pEst, EContextViewState state);
void AddDisableKeyboardMouse(IContextEstablisher* pEst, EContextViewState state);

#endif
