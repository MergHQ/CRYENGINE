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
	if (m_pBaseObject != nullptr)
	{
		m_pBaseObject->RemoveFile(this);
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CBaseStandaloneFile::Play(IObject* const pIObject)
{
	auto const pBaseObject = static_cast<CBaseObject*>(pIObject);

	m_pBaseObject = pBaseObject;
	StartLoading();

	StandaloneFiles& objectPendingFiles = pBaseObject->GetPendingFiles();
	CRY_ASSERT_MESSAGE(std::find(objectPendingFiles.begin(), objectPendingFiles.end(), this) == objectPendingFiles.end(), "Standalone file was already in the pending standalone files list during %s", __FUNCTION__);
	objectPendingFiles.push_back(this);
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CBaseStandaloneFile::Stop(IObject* const pIObject)
{
	StopFile();
	return ERequestStatus::Pending;
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