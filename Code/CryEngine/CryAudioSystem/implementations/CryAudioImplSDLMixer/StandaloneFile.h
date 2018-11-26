// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <IStandaloneFileConnection.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
class CStandaloneFile;

namespace Impl
{
namespace SDL_mixer
{
class CStandaloneFile final : public IStandaloneFileConnection
{
public:

	CStandaloneFile() = delete;
	CStandaloneFile(CStandaloneFile const&) = delete;
	CStandaloneFile(CStandaloneFile&&) = delete;
	CStandaloneFile& operator=(CStandaloneFile const&) = delete;
	CStandaloneFile& operator=(CStandaloneFile&&) = delete;

	explicit CStandaloneFile(char const* const szName, CryAudio::CStandaloneFile& standaloneFile)
		: m_file(standaloneFile)
		, m_name(szName)
	{}

	virtual ~CStandaloneFile() override = default;

	// IStandaloneFileConnection
	virtual ERequestStatus Play(IObject* const pIObject) override;
	virtual ERequestStatus Stop(IObject* const pIObject) override;
	// ~IStandaloneFileConnection

	CryAudio::CStandaloneFile&         m_file;
	SampleId                           m_sampleId = 0; // ID unique to the file, only needed for the 'finished' request
	CryFixedStringT<MaxFilePathLength> m_name;
	ChannelList                        m_channels;
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio