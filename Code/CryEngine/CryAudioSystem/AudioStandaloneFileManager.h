// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
struct IAudioImpl;
}
}

class CAudioStandaloneFileManager
{
public:

	CAudioStandaloneFileManager();
	virtual ~CAudioStandaloneFileManager();

	void                Init(CryAudio::Impl::IAudioImpl* const pImpl);
	void                Release();

	CATLStandaloneFile* GetStandaloneFile(char const* const szFile);
	CATLStandaloneFile* LookupId(AudioStandaloneFileId const instanceId) const;
	void                ReleaseStandaloneFile(CATLStandaloneFile* const pStandaloneFile);

private:

	typedef std::map<AudioStandaloneFileId, CATLStandaloneFile*, std::less<AudioStandaloneFileId>, STLSoundAllocator<std::pair<AudioStandaloneFileId, CATLStandaloneFile*>>>
	  ActiveStandaloneFilesMap;

	ActiveStandaloneFilesMap m_activeAudioStandaloneFiles;

	typedef CInstanceManager<CATLStandaloneFile, AudioStandaloneFileId> AudioStandaloneFilePool;
	AudioStandaloneFilePool     m_audioStandaloneFilePool;

	CryAudio::Impl::IAudioImpl* m_pImpl;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
public:

	void SetDebugNameStore(CATLDebugNameStore* const pDebugNameStore);
	void DrawDebugInfo(IRenderAuxGeom& auxGeom, float posX, float posY) const;

private:

	CATLDebugNameStore* m_pDebugNameStore;
#endif //INCLUDE_AUDIO_PRODUCTION_CODE

	PREVENT_OBJECT_COPY(CAudioStandaloneFileManager);
};
