// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Entity.h"
#include "Common/PoolObject.h"
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
class CPreloadRequest final : public CEntity<PreloadRequestId>, public CPoolObject<CPreloadRequest, stl::PSyncNone>
{
public:

	using FileEntryIds = std::vector<FileEntryId>;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	explicit CPreloadRequest(
		PreloadRequestId const id,
		EDataScope const dataScope,
		bool const bAutoLoad,
		FileEntryIds const& fileEntryIds,
		char const* const szName)
		: CEntity<PreloadRequestId>(id, dataScope, szName)
		, m_bAutoLoad(bAutoLoad)
		, m_fileEntryIds(fileEntryIds)
	{}
#else
	explicit CPreloadRequest(
		PreloadRequestId const id,
		EDataScope const dataScope,
		bool const bAutoLoad,
		FileEntryIds const& fileEntryIds)
		: CEntity<PreloadRequestId>(id, dataScope)
		, m_bAutoLoad(bAutoLoad)
		, m_fileEntryIds(fileEntryIds)
	{}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	bool const   m_bAutoLoad;
	FileEntryIds m_fileEntryIds;
};
} // namespace CryAudio
