// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseStandaloneFile.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CProgrammerSoundFile final : public CBaseStandaloneFile
{
public:

	CProgrammerSoundFile() = delete;
	CProgrammerSoundFile(CProgrammerSoundFile const&) = delete;
	CProgrammerSoundFile(CProgrammerSoundFile&&) = delete;
	CProgrammerSoundFile& operator=(CProgrammerSoundFile const&) = delete;
	CProgrammerSoundFile& operator=(CProgrammerSoundFile&&) = delete;

	explicit CProgrammerSoundFile(
		char const* const szFile,
		FMOD_GUID const eventGuid,
		CryAudio::CStandaloneFile& standaloneFile)
		: CBaseStandaloneFile(szFile, standaloneFile)
		, m_eventGuid(eventGuid)
	{}

	virtual ~CProgrammerSoundFile() override = default;

	// CStandaloneFileBase
	virtual void StartLoading() override;
	virtual bool IsReady() override;
	virtual void Play(FMOD_3D_ATTRIBUTES const& attributes) override;
	virtual void Set3DAttributes(FMOD_3D_ATTRIBUTES const& attributes) override;
	virtual void Stop() override;
	// ~CStandaloneFileBase

private:

	FMOD_GUID const              m_eventGuid;
	FMOD::Studio::EventInstance* m_pEventInstance = nullptr;

};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
