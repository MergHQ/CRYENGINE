// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>
#include <SharedAudioData.h>
#include <PoolObject.h>
#include <AK/SoundEngine/Common/AkTypes.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CObject;

class CEvent final : public IEvent, public CPoolObject<CEvent, stl::PSyncNone>
{
public:

	CEvent() = delete;
	CEvent(CEvent const&) = delete;
	CEvent(CEvent&&) = delete;
	CEvent& operator=(CEvent const&) = delete;
	CEvent& operator=(CEvent&&) = delete;

	explicit CEvent(CATLEvent& atlEvent_);
	virtual ~CEvent() override;

	// CryAudio::Impl::IEvent
	virtual ERequestStatus Stop() override;
	// ~CryAudio::Impl::IEvent

	void SetInitialVirtualState(float const distance);
	void UpdateVirtualState(float const distance);

	EEventState m_state;
	AkUniqueID  m_id;
	CATLEvent&  m_atlEvent;
	CObject*    m_pObject;
	float       m_maxAttenuation;
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
