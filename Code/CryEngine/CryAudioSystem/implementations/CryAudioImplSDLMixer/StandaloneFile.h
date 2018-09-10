// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <ATLEntityData.h>

namespace CryAudio
{
class CATLStandaloneFile;

namespace Impl
{
namespace SDL_mixer
{
class CStandaloneFile final : public IStandaloneFile
{
public:

	CStandaloneFile() = delete;
	CStandaloneFile(CStandaloneFile const&) = delete;
	CStandaloneFile(CStandaloneFile&&) = delete;
	CStandaloneFile& operator=(CStandaloneFile const&) = delete;
	CStandaloneFile& operator=(CStandaloneFile&&) = delete;

	explicit CStandaloneFile(char const* const szName, CATLStandaloneFile& atlStandaloneFile);
	virtual ~CStandaloneFile() override = default;

	CATLStandaloneFile&                m_atlFile;
	SampleId                           m_sampleId = 0; // ID unique to the file, only needed for the 'finished' request
	CryFixedStringT<MaxFilePathLength> m_name;
	ChannelList                        m_channels;
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio