// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IParameterConnection.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CAisacControlAdvanced final : public IParameterConnection, public CPoolObject<CAisacControlAdvanced, stl::PSyncNone>
{
public:

	CAisacControlAdvanced() = delete;
	CAisacControlAdvanced(CAisacControlAdvanced const&) = delete;
	CAisacControlAdvanced(CAisacControlAdvanced&&) = delete;
	CAisacControlAdvanced& operator=(CAisacControlAdvanced const&) = delete;
	CAisacControlAdvanced& operator=(CAisacControlAdvanced&&) = delete;

#if defined(CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE)
	explicit CAisacControlAdvanced(
		CriAtomExAisacControlId const id,
		float const minValue,
		float const maxValue,
		float const multiplier,
		float const shift,
		char const* const szName)
		: m_id(id)
		, m_minValue(minValue)
		, m_maxValue(maxValue)
		, m_multiplier(multiplier)
		, m_shift(shift)
		, m_name(szName)
	{}
#else
	explicit CAisacControlAdvanced(
		CriAtomExAisacControlId const id,
		float const minValue,
		float const maxValue,
		float const multiplier,
		float const shift)
		: m_id(id)
		, m_minValue(minValue)
		, m_maxValue(maxValue)
		, m_multiplier(multiplier)
		, m_shift(shift)
	{}
#endif  // CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE

	virtual ~CAisacControlAdvanced() override = default;

	// IParameterConnection
	virtual void Set(IObject* const pIObject, float const value) override;
	virtual void SetGlobally(float const value) override;
	// ~IParameterConnection

private:

	CriAtomExAisacControlId const m_id;
	float const                   m_minValue;
	float const                   m_maxValue;
	float const                   m_multiplier;
	float const                   m_shift;

#if defined(CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE)
	CryFixedStringT<MaxControlNameLength> const m_name;
#endif  // CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
