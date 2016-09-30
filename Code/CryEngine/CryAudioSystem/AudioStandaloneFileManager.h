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

class CAudioStandaloneFileManager final
{
public:

	CAudioStandaloneFileManager(AudioStandaloneFileLookup& audioStandaloneFiles);
	~CAudioStandaloneFileManager();

	CAudioStandaloneFileManager(CAudioStandaloneFileManager const&) = delete;
	CAudioStandaloneFileManager(CAudioStandaloneFileManager&&) = delete;
	CAudioStandaloneFileManager& operator=(CAudioStandaloneFileManager const&) = delete;
	CAudioStandaloneFileManager& operator=(CAudioStandaloneFileManager&&) = delete;

	void                Init(CryAudio::Impl::IAudioImpl* const pImpl);
	void                Release();

	CATLStandaloneFile* GetStandaloneFile(char const* const szFile);
	CATLStandaloneFile* LookupId(AudioStandaloneFileId const instanceId) const;
	void                ReleaseStandaloneFile(CATLStandaloneFile* const pStandaloneFile);

private:

	AudioStandaloneFileLookup& m_audioStandaloneFiles;

	typedef CInstanceManager<CATLStandaloneFile, AudioStandaloneFileId> AudioStandaloneFilePool;
	AudioStandaloneFilePool     m_audioStandaloneFilePool;

	CryAudio::Impl::IAudioImpl* m_pImpl;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
public:

	void DrawDebugInfo(IRenderAuxGeom& auxGeom, float posX, float posY) const;

#endif //INCLUDE_AUDIO_PRODUCTION_CODE
};
