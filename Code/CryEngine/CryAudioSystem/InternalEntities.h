// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"

namespace CryAudio
{
class CATLAudioObject;

struct SInternalControls
{
	using SwitchState = std::pair<ControlId const, SwitchStateId const>;

	std::map<SwitchState, IAudioSwitchStateImpl const*> m_switchStates;
};

class COcclusionObstructionState final : public IAudioSwitchStateImpl
{
public:

	COcclusionObstructionState() = delete;
	explicit COcclusionObstructionState(SwitchStateId const stateId);

	COcclusionObstructionState(COcclusionObstructionState const&) = delete;
	COcclusionObstructionState(COcclusionObstructionState&&) = delete;
	COcclusionObstructionState& operator=(COcclusionObstructionState const&) = delete;
	COcclusionObstructionState& operator=(COcclusionObstructionState&&) = delete;

	// IAudioSwitchStateImpl
	virtual void Set(CATLAudioObject& audioObject) const override;
	// ~IAudioSwitchStateImpl

private:

	SwitchStateId const m_stateId;
};

class CRelativeVelocityTrackingState final : public IAudioSwitchStateImpl
{
public:

	CRelativeVelocityTrackingState() = delete;
	explicit CRelativeVelocityTrackingState(SwitchStateId const stateId);

	CRelativeVelocityTrackingState(CRelativeVelocityTrackingState const&) = delete;
	CRelativeVelocityTrackingState(CRelativeVelocityTrackingState&&) = delete;
	CRelativeVelocityTrackingState& operator=(CRelativeVelocityTrackingState const&) = delete;
	CRelativeVelocityTrackingState& operator=(CRelativeVelocityTrackingState&&) = delete;

	// IAudioSwitchStateImpl
	virtual void Set(CATLAudioObject& audioObject) const override;
	// ~IAudioSwitchStateImpl

private:

	SwitchStateId const m_stateId;
};

class CAbsoluteVelocityTrackingState final : public IAudioSwitchStateImpl
{
public:

	CAbsoluteVelocityTrackingState() = delete;
	explicit CAbsoluteVelocityTrackingState(SwitchStateId const stateId);

	CAbsoluteVelocityTrackingState(CAbsoluteVelocityTrackingState const&) = delete;
	CAbsoluteVelocityTrackingState(CAbsoluteVelocityTrackingState&&) = delete;
	CAbsoluteVelocityTrackingState& operator=(CAbsoluteVelocityTrackingState const&) = delete;
	CAbsoluteVelocityTrackingState& operator=(CAbsoluteVelocityTrackingState&&) = delete;

	// IAudioSwitchStateImpl
	virtual void Set(CATLAudioObject& audioObject) const override;
	// ~IAudioSwitchStateImpl

private:

	SwitchStateId const m_stateId;
};

class CDoNothingTrigger final : public CATLTriggerImpl
{
public:

	CDoNothingTrigger() = delete;

	explicit CDoNothingTrigger(TriggerImplId const id)
		: CATLTriggerImpl(id)
	{}

	CDoNothingTrigger(CDoNothingTrigger const&) = delete;
	CDoNothingTrigger(CDoNothingTrigger&&) = delete;
	CDoNothingTrigger& operator=(CDoNothingTrigger const&) = delete;
	CDoNothingTrigger& operator=(CDoNothingTrigger&&) = delete;

	// CATLTriggerImpl
	virtual ERequestStatus Execute(Impl::IObject* const pImplObject, Impl::IEvent* const pImplEvent) const override;
	// ~CATLTriggerImpl
};
} // namespace CryAudio
