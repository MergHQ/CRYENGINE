// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
struct IRenderAuxGeom;
#endif //CRY_AUDIO_USE_PRODUCTION_CODE

namespace CryAudio
{
class CStandaloneFile;

namespace Impl
{
struct ITriggerConnection;
} // namespace Impl

class CFileManager final
{
public:

	CFileManager() = default;
	~CFileManager();

	CFileManager(CFileManager const&) = delete;
	CFileManager(CFileManager&&) = delete;
	CFileManager&    operator=(CFileManager const&) = delete;
	CFileManager&    operator=(CFileManager&&) = delete;

	void             ReleaseImplData();
	void             Release();
	CStandaloneFile* ConstructStandaloneFile(char const* const szFile, bool const bLocalized, Impl::ITriggerConnection const* const pITriggerConnection = nullptr);
	void             ReleaseStandaloneFile(CStandaloneFile* const pStandaloneFile);

private:

	std::vector<CStandaloneFile*> m_constructedStandaloneFiles;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
public:

	void DrawDebugInfo(IRenderAuxGeom& auxGeom, float posX, float posY) const;

#endif // CRY_AUDIO_USE_PRODUCTION_CODE
};
} // namespace CryAudio
