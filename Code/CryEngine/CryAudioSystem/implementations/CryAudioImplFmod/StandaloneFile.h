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

	explicit CStandaloneFile(char const* const szFile, CATLStandaloneFile& atlStandaloneFile);
	virtual ~CStandaloneFile() override = default;

	// CStandaloneFileBase
	virtual void StartLoading() override;
	virtual bool IsReady() override;
	virtual void Play(FMOD_3D_ATTRIBUTES const& attributes) override;
	virtual void Set3DAttributes(FMOD_3D_ATTRIBUTES const& attributes) override;
	virtual void Stop() override;
	// ~CStandaloneFileBase

	FMOD::Sound*   m_pLowLevelSound = nullptr;
	FMOD::Channel* m_pChannel = nullptr;
};

using StandaloneFiles = std::vector<CStandaloneFile*>;
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
