// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   ILipSyncProvider.h
//  Version:     v1.00
//  Created:     2014-08-29 by Christian Werle.
//  Description: Interface for the lip-sync provider that gets injected into the IEntitySoundProxy.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#pragma once

enum ELipSyncMethod
{
	eLSM_None,
	eLSM_MatchAnimationToSoundName,
};

struct IEntityAudioProxy;

struct ILipSyncProvider
{
	virtual ~ILipSyncProvider() {}

	//! Use this to start loading as soon as the sound gets requested (this can be safely ignored).
	virtual void RequestLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod) = 0;

	virtual void StartLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod) = 0;
	virtual void PauseLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod) = 0;
	virtual void UnpauseLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod) = 0;
	virtual void StopLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod) = 0;
	virtual void UpdateLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod) = 0;
};

DECLARE_SHARED_POINTERS(ILipSyncProvider);
