// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>

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

class CParameter : public IParameter
{
public:

	CParameter() = delete;
	CParameter(CParameter const&) = delete;
	CParameter(CParameter&&) = delete;
	CParameter& operator=(CParameter const&) = delete;
	CParameter& operator=(CParameter&&) = delete;

	explicit CParameter(
		uint32 const id,
		float const multiplier,
		float const shift,
		char const* const szName,
		EParameterType const type);

	virtual ~CParameter() override = default;

	uint32                                       GetId() const              { return m_id; }
	float                                        GetValueMultiplier() const { return m_multiplier; }
	float                                        GetValueShift() const      { return m_shift; }
	EParameterType                               GetType() const            { return m_type; }
	CryFixedStringT<MaxControlNameLength> const& GetName() const            { return m_name; }

private:

	uint32 const                                m_id;
	float const                                 m_multiplier;
	float const                                 m_shift;
	EParameterType const                        m_type;
	CryFixedStringT<MaxControlNameLength> const m_name;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
