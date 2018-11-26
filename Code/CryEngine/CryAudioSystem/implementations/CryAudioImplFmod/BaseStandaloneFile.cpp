// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "BaseStandaloneFile.h"
#include "BaseObject.h"

#include <Logger.h>
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
ERequestStatus CBaseStandaloneFile::Play(IObject* const pIObject)
{
	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CBaseObject*>(pIObject);

		m_pObject = pObject;
		StartLoading();

		StandaloneFiles& objectPendingFiles = pObject->GetPendingFiles();
		CRY_ASSERT_MESSAGE(std::find(objectPendingFiles.begin(), objectPendingFiles.end(), this) == objectPendingFiles.end(), "Standalone file was already in the pending standalone files list during %s", __FUNCTION__);
		objectPendingFiles.push_back(this);
		return ERequestStatus::Success;
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid standalone file pointer passed to the Fmod implementation of %s.", __FUNCTION__);
	}

	return ERequestStatus::Failure;
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