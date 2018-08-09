// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseConnection.h"

#include <CryAudioImplAdx2/GlobalData.h>

namespace ACE
{
namespace Impl
{
namespace Adx2
{
class CParameterConnection final : public CBaseConnection
{
public:

	CParameterConnection() = delete;

	explicit CParameterConnection(
		ControlId const id,
		float const mult = CryAudio::Impl::Adx2::s_defaultParamMultiplier,
		float const shift = CryAudio::Impl::Adx2::s_defaultParamShift)
		: CBaseConnection(id)
		, m_mult(mult)
		, m_shift(shift)
	{}

	// CBaseConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CBaseConnection

	float GetMultiplier() const { return m_mult; }
	float GetShift() const      { return m_shift; }

private:

	float m_mult;
	float m_shift;
};
} // namespace Adx2
} // namespace Impl
} // namespace ACE
