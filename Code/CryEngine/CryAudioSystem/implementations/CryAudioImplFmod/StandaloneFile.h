// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseStandaloneFile.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CStandaloneFile final : public CBaseStandaloneFile
{
public:

	CStandaloneFile() = delete;
	CStandaloneFile(CStandaloneFile const&) = delete;
	CStandaloneFile(CStandaloneFile&&) = delete;
	CStandaloneFile& operator=(CStandaloneFile const&) = delete;
	CStandaloneFile& operator=(CStandaloneFile&&) = delete;

	explicit CStandaloneFile(char const* const szFile, CryAudio::CStandaloneFile& standaloneFile)
		: CBaseStandaloneFile(szFile, standaloneFile)
	{}

	virtual ~CStandaloneFile() override = default;

	// CStandaloneFileBase
	virtual bool IsReady() override;
	virtual void PlayFile(FMOD_3D_ATTRIBUTES const& attributes) override;
	virtual void Set3DAttributes(FMOD_3D_ATTRIBUTES const& attributes) override;
	// ~CStandaloneFileBase

	FMOD::Channel* m_pChannel = nullptr;

private:

	// CStandaloneFileBase
	virtual void StartLoading() override;
	virtual void StopFile() override;
	// ~CStandaloneFileBase

	FMOD::Sound* m_pLowLevelSound = nullptr;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
