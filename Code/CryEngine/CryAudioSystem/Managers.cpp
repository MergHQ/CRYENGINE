// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Managers.h"
#include "AudioEventManager.h"
#include "AudioStandaloneFileManager.h"
#include "AudioObjectManager.h"
#include "AudioListenerManager.h"
#include "FileCacheManager.h"
#include "AudioEventListenerManager.h"
#include "AudioXMLProcessor.h"

namespace CryAudio
{
CEventManager g_eventManager;
CFileManager g_fileManager;
CObjectManager g_objectManager;
CAudioListenerManager g_listenerManager;
CFileCacheManager g_fileCacheManager;
CAudioEventListenerManager g_eventListenerManager;
CAudioXMLProcessor g_xmlProcessor;
} // namespace CryAudio
