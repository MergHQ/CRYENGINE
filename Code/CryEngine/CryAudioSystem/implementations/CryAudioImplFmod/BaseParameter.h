// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IParameterConnection.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
enum class EParameterType : EnumFlagsType
{
	None,
	Parameter,
	VCA,
};

class CBaseParameter : public IParameterConnection
{
public:

	CBaseParameter() = delete;
	CBaseParameter(CBaseParameter const&) = delete;
	CBaseParameter(CBaseParameter&&) = delete;
	CBaseParameter& operator=(CBaseParameter const&) = delete;
	CBaseParameter& operator=(CBaseParameter&&) = delete;

	virtual ~CBaseParameter() override = default;

	uint32         GetId() const              { return m_id; }
	float          GetValueMultiplier() const { return m_multiplier; }
	float          GetValueShift() const      { return m_shift; }
	EParameterType GetType() const            { return m_type; }

protected:

	explicit CBaseParameter(
		uint32 const id,
		float const multiplier,
		float const shift,
		EParameterType const type)
		: m_id(id)
		, m_multiplier(multiplier)
		, m_shift(shift)
		, m_type(type)
	{}

	uint32 const         m_id;
	float const          m_multiplier;
	float const          m_shift;
	EParameterType const m_type;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
