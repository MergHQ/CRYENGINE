// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
struct IRenderAuxGeom;
#endif //INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
class CATLStandaloneFile;

namespace Impl
{
struct ITrigger;
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

	void                         Release();
	CATLStandaloneFile*          ConstructStandaloneFile(char const* const szFile, bool const bLocalized, Impl::ITrigger const* const pITrigger = nullptr);
	void                         ReleaseStandaloneFile(CATLStandaloneFile* const pStandaloneFile);

private:

	std::list<CATLStandaloneFile*> m_constructedStandaloneFiles;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
public:

	void DrawDebugInfo(IRenderAuxGeom& auxGeom, Vec3 const& listenerPosition, float posX, float posY) const;

#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};
} // namespace CryAudio
