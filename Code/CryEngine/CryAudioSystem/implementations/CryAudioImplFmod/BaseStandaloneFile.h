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
namespace Fmod
{
class CBaseObject;

class CBaseStandaloneFile : public IStandaloneFileConnection
{
public:

	CBaseStandaloneFile() = delete;
	CBaseStandaloneFile(CBaseStandaloneFile const&) = delete;
	CBaseStandaloneFile(CBaseStandaloneFile&&) = delete;
	CBaseStandaloneFile& operator=(CBaseStandaloneFile const&) = delete;
	CBaseStandaloneFile& operator=(CBaseStandaloneFile&&) = delete;

	virtual ~CBaseStandaloneFile() override;

	// IStandaloneFileConnection
	virtual ERequestStatus Play(IObject* const pIObject) override;
	virtual ERequestStatus Stop(IObject* const pIObject) override;
	// ~IStandaloneFileConnection

	virtual bool IsReady() = 0;
	virtual void PlayFile(FMOD_3D_ATTRIBUTES const& attributes) = 0;
	virtual void Set3DAttributes(FMOD_3D_ATTRIBUTES const& attributes) = 0;

	void         ReportFileStarted();
	void         ReportFileFinished();

	char const*  GetFileName() const { return m_fileName.c_str(); }

	static FMOD::System* s_pLowLevelSystem;

protected:

	explicit CBaseStandaloneFile(char const* const szFile, CryAudio::CStandaloneFile& standaloneFile)
		: m_standaloneFile(standaloneFile)
		, m_fileName(szFile)
	{}

	virtual void StartLoading() = 0;
	virtual void StopFile() = 0;

	CryAudio::CStandaloneFile&         m_standaloneFile;
	CryFixedStringT<MaxFilePathLength> m_fileName;
	CBaseObject*                       m_pBaseObject = nullptr;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
