// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "StandaloneFile.h"

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
//////////////////////////////////////////////////////////////////////////
CStandaloneFile::CStandaloneFile(char const* const szName, CATLStandaloneFile& atlStandaloneFile)
	: m_atlFile(atlStandaloneFile)
	, m_name(szName)
{
}
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio
