// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Environment.h"
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CEnvironmentBus final : public CEnvironment, public CPoolObject<CEnvironmentBus, stl::PSyncNone>
{
public:

	CEnvironmentBus() = delete;
	CEnvironmentBus(CEnvironmentBus const&) = delete;
	CEnvironmentBus(CEnvironmentBus&&) = delete;
	CEnvironmentBus& operator=(CEnvironmentBus const&) = delete;
	CEnvironmentBus& operator=(CEnvironmentBus&&) = delete;

	explicit CEnvironmentBus(
		FMOD::Studio::EventDescription* const pEventDescription,
		FMOD::Studio::Bus* const pBus)
		: CEnvironment(EEnvironmentType::Bus)
		, m_pEventDescription(pEventDescription)
		, m_pBus(pBus)
	{}

	virtual ~CEnvironmentBus() override = default;

	FMOD::Studio::Bus* GetBus() const { return m_pBus; }

private:

	FMOD::Studio::EventDescription* const m_pEventDescription;
	FMOD::Studio::Bus* const              m_pBus;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
