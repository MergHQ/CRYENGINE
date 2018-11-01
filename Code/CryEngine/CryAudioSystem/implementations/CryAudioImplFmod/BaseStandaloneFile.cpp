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
	gEnv->pAudioSystem->ReportStartedFile(m_standaloneFile, true);
}

//////////////////////////////////////////////////////////////////////////
void CBaseStandaloneFile::ReportFileFinished()
{
	gEnv->pAudioSystem->ReportStoppedFile(m_standaloneFile);
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio