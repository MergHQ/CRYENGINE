// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"

namespace CryAudio
{
class CAudioListenerManager;
class CATLAudioObject;

struct SInternalControls
{
	using SwitchState = std::pair<ControlId const, SwitchStateId const>;

	std::map<SwitchState, IAudioSwitchStateImpl const*> m_switchStates;
	std::map<ControlId, IParameterImpl*>                m_parameters;
};

class COcclusionObstructionState final : public IAudioSwitchStateImpl
{
public:

	explicit COcclusionObstructionState(SwitchStateId const stateId, CAudioListenerManager const& audioListenerManager, CATLAudioObject const& globalAudioObject);
	virtual ERequestStatus Set(CATLAudioObject& audioObject) const override;

	COcclusionObstructionState(COcclusionObstructionState const&) = delete;
	COcclusionObstructionState(COcclusionObstructionState&&) = delete;
	COcclusionObstructionState& operator=(COcclusionObstructionState const&) = delete;
	COcclusionObstructionState& operator=(COcclusionObstructionState&&) = delete;

private:

	SwitchStateId const          m_stateId;
	CAudioListenerManager const& m_audioListenerManager;
	CATLAudioObject const&       m_globalAudioObject;
};

class CRelativeVelocityTrackingState final : public IAudioSwitchStateImpl
{
public:

	explicit CRelativeVelocityTrackingState(SwitchStateId const stateId, CATLAudioObject const& globalAudioObject);
	virtual ERequestStatus Set(CATLAudioObject& audioObject) const override;

	CRelativeVelocityTrackingState(CRelativeVelocityTrackingState const&) = delete;
	CRelativeVelocityTrackingState(CRelativeVelocityTrackingState&&) = delete;
	CRelativeVelocityTrackingState& operator=(CRelativeVelocityTrackingState const&) = delete;
	CRelativeVelocityTrackingState& operator=(CRelativeVelocityTrackingState&&) = delete;

private:

	SwitchStateId const    m_stateId;
	CATLAudioObject const& m_globalAudioObject;
};

class CAbsoluteVelocityTrackingState final : public IAudioSwitchStateImpl
{
public:

	explicit CAbsoluteVelocityTrackingState(SwitchStateId const stateId, CATLAudioObject const& globalAudioObject);
	virtual ERequestStatus Set(CATLAudioObject& audioObject) const override;

	CAbsoluteVelocityTrackingState(CAbsoluteVelocityTrackingState const&) = delete;
	CAbsoluteVelocityTrackingState(CAbsoluteVelocityTrackingState&&) = delete;
	CAbsoluteVelocityTrackingState& operator=(CAbsoluteVelocityTrackingState const&) = delete;
	CAbsoluteVelocityTrackingState& operator=(CAbsoluteVelocityTrackingState&&) = delete;

private:

	SwitchStateId const    m_stateId;
	CATLAudioObject const& m_globalAudioObject;
};
} // namespace CryAudio
