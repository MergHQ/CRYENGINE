// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IAudioImpl.h>
#include "AudioInternalInterfaces.h"

class CAudioEventListenerManager
{
public:

	CAudioEventListenerManager();
	~CAudioEventListenerManager();

	EAudioRequestStatus AddRequestListener(SAudioManagerRequestDataInternal<eAudioManagerRequestType_AddRequestListener> const* const pRequestData);
	EAudioRequestStatus RemoveRequestListener(void (* func)(SAudioRequestInfo const* const), void const* const pObjectToListenTo);
	void                NotifyListener(SAudioRequestInfo const* const pRequestInfo);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	size_t GetNumEventListeners() const { return m_listeners.size(); }
#endif //INCLUDE_AUDIO_PRODUCTION_CODE

private:

	typedef std::vector<SAudioEventListener, STLSoundAllocator<SAudioEventListener>> TListenerArray;
	TListenerArray m_listeners;

	PREVENT_OBJECT_COPY(CAudioEventListenerManager);
};
