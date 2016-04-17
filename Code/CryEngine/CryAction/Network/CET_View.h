// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __CET_VIEW_H__
#define __CET_VIEW_H__

#pragma once

void AddInitView_ClientActor(IContextEstablisher* pEst, EContextViewState state);
void AddInitView_CurrentSpectator(IContextEstablisher* pEst, EContextViewState state);
void AddClearViews(IContextEstablisher* pEst, EContextViewState state);

#endif
