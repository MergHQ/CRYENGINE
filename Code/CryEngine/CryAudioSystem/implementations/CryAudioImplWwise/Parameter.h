// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IParameterConnection.h>
#include <PoolObject.h>
#include <AK/SoundEngine/Common/AkTypes.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CParameter final : public IParameterConnection, public CPoolObject<CParameter, stl::PSyncNone>
{
public:

	CParameter() = delete;
	CParameter(CParameter const&) = delete;
	CParameter(CParameter&&) = delete;
	CParameter& operator=(CParameter const&) = delete;
	CParameter& operator=(CParameter&&) = delete;

	explicit CParameter(AkRtpcID const id, float const mult, float const shift)
		: m_multiplier(mult)
		, m_shift(shift)
		, m_id(id)
	{}

	virtual ~CParameter() override = default;

	// IParameterConnection
	virtual void Set(IObject* const pIObject, float const value) override;
	// ~IParameterConnection

	float    GetMultiplier() const { return m_multiplier; }
	float    GetShift() const      { return m_shift; }
	AkRtpcID GetId() const         { return m_id; }

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	void SetName(char const* const szName) { m_name = szName; }
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

private:

	float const    m_multiplier;
	float const    m_shift;
	AkRtpcID const m_id;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> m_name;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
