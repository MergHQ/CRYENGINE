// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseConnection.h"

#include <PoolObject.h>
#include <CryAudioImplSDLMixer/GlobalData.h>

namespace ACE
{
namespace Impl
{
namespace SDLMixer
{
class CStateConnection final : public CBaseConnection, public CryAudio::CPoolObject<CStateConnection, stl::PSyncNone>
{
public:

	CStateConnection() = delete;
	CStateConnection(CStateConnection const&) = delete;
	CStateConnection(CStateConnection&&) = delete;
	CStateConnection& operator=(CStateConnection const&) = delete;
	CStateConnection& operator=(CStateConnection&&) = delete;

	explicit CStateConnection(
		ControlId const id,
		float const value = CryAudio::Impl::SDL_mixer::g_defaultStateValue)
		: CBaseConnection(id)
		, m_value(value)
	{}

	virtual ~CStateConnection() override = default;

	// CBaseConnection
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CBaseConnection

	float GetValue() const { return m_value; }

private:

	float m_value;
};
} // namespace SDLMixer
} // namespace Impl
} // namespace ACE
