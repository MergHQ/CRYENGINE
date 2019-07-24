// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEnvironmentConnection.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CAisacEnvironmentAdvanced final : public IEnvironmentConnection, public CPoolObject<CAisacEnvironmentAdvanced, stl::PSyncNone>
{
public:

	CAisacEnvironmentAdvanced() = delete;
	CAisacEnvironmentAdvanced(CAisacEnvironmentAdvanced const&) = delete;
	CAisacEnvironmentAdvanced(CAisacEnvironmentAdvanced&&) = delete;
	CAisacEnvironmentAdvanced& operator=(CAisacEnvironmentAdvanced const&) = delete;
	CAisacEnvironmentAdvanced& operator=(CAisacEnvironmentAdvanced&&) = delete;

#if defined(CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE)
	explicit CAisacEnvironmentAdvanced(
		CriAtomExAisacControlId const id,
		float const multiplier,
		float const shift,
		char const* const szName)
		: m_id(id)
		, m_multiplier(multiplier)
		, m_shift(shift)
		, m_name(szName)
	{}
#else
	explicit CAisacEnvironmentAdvanced(
		CriAtomExAisacControlId const id,
		float const multiplier,
		float const shift)
		: m_id(id)
		, m_multiplier(multiplier)
		, m_shift(shift)
	{}
#endif  // CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE

	virtual ~CAisacEnvironmentAdvanced() override = default;

	// IEnvironmentConnection
	virtual void Set(IObject* const pIObject, float const amount) override;
	// ~IEnvironmentConnection

private:

	CriAtomExAisacControlId const m_id;
	float const                   m_multiplier;
	float const                   m_shift;

#if defined(CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE)
	CryFixedStringT<MaxControlNameLength> const m_name;
#endif  // CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
