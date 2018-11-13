// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

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
enum class EParameterType : EnumFlagsType
{
	None,
	AisacControl,
	Category,
	GameVariable,
};

class CParameter final : public IParameterConnection, public CPoolObject<CParameter, stl::PSyncNone>
{
public:

	CParameter() = delete;
	CParameter(CParameter const&) = delete;
	CParameter(CParameter&&) = delete;
	CParameter& operator=(CParameter const&) = delete;
	CParameter& operator=(CParameter&&) = delete;

	explicit CParameter(
		char const* const szName,
		EParameterType const type,
		float const multiplier,
		float const shift)
		: m_name(szName)
		, m_type(type)
		, m_multiplier(multiplier)
		, m_shift(shift)
	{}

	virtual ~CParameter() override = default;

	CriChar8 const* GetName() const       { return static_cast<CriChar8 const*>(m_name); }
	EParameterType  GetType() const       { return m_type; }
	float           GetMultiplier() const { return m_multiplier; }
	float           GetValueShift() const { return m_shift; }

private:

	CryFixedStringT<MaxControlNameLength> const m_name;
	EParameterType const                        m_type;
	float const m_multiplier;
	float const m_shift;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
