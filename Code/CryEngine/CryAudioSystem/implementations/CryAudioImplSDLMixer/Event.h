// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
class CTrigger;

class CEvent final : public CPoolObject<CEvent, stl::PSyncNone>
{
public:

	CEvent() = delete;
	CEvent(CEvent const&) = delete;
	CEvent(CEvent&&) = delete;
	CEvent& operator=(CEvent const&) = delete;
	CEvent& operator=(CEvent&&) = delete;

	explicit CEvent(TriggerInstanceId const triggerInstanceId)
		: m_triggerInstanceId(triggerInstanceId)
		, m_triggerId(InvalidCRC32)
		, m_pObject(nullptr)
		, m_pTrigger(nullptr)
		, m_toBeRemoved(false)
	{}

	~CEvent() = default;

	void Stop();
	void Pause();
	void Resume();

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	void        SetName(char const* const szName) { m_name = szName; }
	char const* GetName() const                   { return m_name.c_str(); }
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE

	TriggerInstanceId const m_triggerInstanceId;
	uint32                  m_triggerId;
	ChannelList             m_channels;
	CObject*                m_pObject;
	CTrigger const*         m_pTrigger;
	bool                    m_toBeRemoved;

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
private:

	CryFixedStringT<MaxControlNameLength> m_name;
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio