// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "BaseStandaloneFile.h"
#include "BaseObject.h"

#include <CryAudio/IAudioSystem.h>
#include <CrySystem/ISystem.h> // needed for gEnv in Release builds

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CBaseStandaloneFile::CBaseStandaloneFile(char const* const szFile, CATLStandaloneFile& atlStandaloneFile)
	: m_atlStandaloneFile(atlStandaloneFile)
	, m_fileName(szFile)
{
}

//////////////////////////////////////////////////////////////////////////
CBaseStandaloneFile::~CBaseStandaloneFile()
{
	if (m_pObject != nullptr)
	{
		m_pObject->RemoveFile(this);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseStandaloneFile::ReportFileStarted()
{
	gEnv->pAudioSystem->ReportStartedFile(m_atlStandaloneFile, true);
}

//////////////////////////////////////////////////////////////////////////
void CBaseStandaloneFile::ReportFileFinished()
{
	gEnv->pAudioSystem->ReportStoppedFile(m_atlStandaloneFile);
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio