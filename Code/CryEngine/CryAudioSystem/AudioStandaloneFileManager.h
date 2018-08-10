// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

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

class CFileManager final
{
public:

	CFileManager() = default;
	~CFileManager();

	CFileManager(CFileManager const&) = delete;
	CFileManager(CFileManager&&) = delete;
	CFileManager&       operator=(CFileManager const&) = delete;
	CFileManager&       operator=(CFileManager&&) = delete;

	void                ReleaseImplData();
	void                Release();
	CATLStandaloneFile* ConstructStandaloneFile(char const* const szFile, bool const bLocalized, Impl::ITrigger const* const pITrigger = nullptr);
	void                ReleaseStandaloneFile(CATLStandaloneFile* const pStandaloneFile);

private:

	std::vector<CATLStandaloneFile*> m_constructedStandaloneFiles;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
public:

	void DrawDebugInfo(IRenderAuxGeom& auxGeom, float posX, float posY) const;

#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};
} // namespace CryAudio
