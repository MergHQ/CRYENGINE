// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <IEvent.h>
#include <PoolObject.h>

namespace CryAudio
{
class CEvent;

namespace Impl
{
namespace SDL_mixer
{
class CTrigger;

class CEvent final : public IEvent, public CPoolObject<CEvent, stl::PSyncNone>
{
public:

	CEvent() = delete;
	CEvent(CEvent const&) = delete;
	CEvent(CEvent&&) = delete;
	CEvent& operator=(CEvent const&) = delete;
	CEvent& operator=(CEvent&&) = delete;

	explicit CEvent(CryAudio::CEvent& event)
		: m_event(event)
	{}

	virtual ~CEvent() override = default;

	// CryAudio::Impl::IEvent
	virtual ERequestStatus Stop() override;
	// ~CryAudio::Impl::IEvent

	void Pause();
	void Resume();

	CryAudio::CEvent& m_event;
	ChannelList       m_channels;
	CTrigger const*   m_pTrigger = nullptr;
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio