// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GlobalData.h"
#include <ATLEntityData.h>
#include <PoolObject.h>

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
enum class EEnvironmentType : EnumFlagsType
{
	None,
	Bus,
	AisacControl,
};

class CEnvironment final : public IEnvironment, public CPoolObject<CEnvironment, stl::PSyncNone>
{
public:

	CEnvironment() = delete;
	CEnvironment(CEnvironment const&) = delete;
	CEnvironment(CEnvironment&&) = delete;
	CEnvironment& operator=(CEnvironment const&) = delete;
	CEnvironment& operator=(CEnvironment&&) = delete;

	explicit CEnvironment(
		char const* const szName,
		EEnvironmentType const type,
		float const multiplier = s_defaultParamMultiplier,
		float const shift = s_defaultParamShift)
		: m_name(szName)
		, m_type(type)
		, m_multiplier(multiplier)
		, m_shift(shift)
	{}

	virtual ~CEnvironment() override = default;

	CriChar8 const*  GetName() const       { return static_cast<CriChar8 const*>(m_name); }
	EEnvironmentType GetType() const       { return m_type; }
	float            GetMultiplier() const { return m_multiplier; }
	float            GetShift() const      { return m_shift; }

private:

	CryFixedStringT<MaxControlNameLength> const m_name;
	EEnvironmentType const                      m_type;
	float const m_multiplier;
	float const m_shift;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
