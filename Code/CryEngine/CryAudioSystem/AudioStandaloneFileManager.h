// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
struct IAudioImpl;
struct IAudioTrigger;
} // namespace Impl

class CAudioStandaloneFileManager final
{
public:

	CAudioStandaloneFileManager() = default;
	~CAudioStandaloneFileManager();

	CAudioStandaloneFileManager(CAudioStandaloneFileManager const&) = delete;
	CAudioStandaloneFileManager(CAudioStandaloneFileManager&&) = delete;
	CAudioStandaloneFileManager& operator=(CAudioStandaloneFileManager const&) = delete;
	CAudioStandaloneFileManager& operator=(CAudioStandaloneFileManager&&) = delete;

	void                         Init(Impl::IAudioImpl* const pImpl);
	void                         Release();

	CATLStandaloneFile*          ConstructStandaloneFile(char const* const szFile, bool const bLocalized, Impl::IAudioTrigger const* const pTriggerImpl = nullptr);
	void                         ReleaseStandaloneFile(CATLStandaloneFile* const pStandaloneFile);

private:

	std::list<CATLStandaloneFile*> m_constructedStandaloneFiles;

	Impl::IAudioImpl*    m_pImpl = nullptr;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
public:

	void DrawDebugInfo(IRenderAuxGeom& auxGeom, float posX, float posY) const;

#endif //INCLUDE_AUDIO_PRODUCTION_CODE
};
} // namespace CryAudio
