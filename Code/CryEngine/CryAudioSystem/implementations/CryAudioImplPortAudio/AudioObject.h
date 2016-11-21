// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
class CAudioEvent;

class CAudioObject final : public IAudioObject
{
public:

	CAudioObject() = default;
	virtual ~CAudioObject() override = default;

	CAudioObject(CAudioObject const&) = delete;
	CAudioObject(CAudioObject&&) = delete;
	CAudioObject& operator=(CAudioObject const&) = delete;
	CAudioObject& operator=(CAudioObject&&) = delete;

	void          Update();
	void          StopAudioEvent(uint32 const pathId);
	void          RegisterAudioEvent(CAudioEvent* const pPAAudioEvent);
	void          UnregisterAudioEvent(CAudioEvent* const pPAAudioEvent);

private:

	std::unordered_map<AudioEventId, CAudioEvent*> m_activeEvents;
};
}
}
}
