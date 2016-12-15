// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ClassRegistryReplicator.h"

void AddOnClientConnect(IContextEstablisher* pEst, EContextViewState state, bool isReset);
void AddOnClientEnteredGame(IContextEstablisher* pEst, EContextViewState state, bool isReset);