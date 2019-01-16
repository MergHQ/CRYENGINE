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

	// IParameterConnection
	virtual void Set(IObject* const pIObject, float const value) override {}
	virtual void SetGlobally(float const value) override                  {}
	// ~IParameterConnection

	uint32 GetId() const { return m_id; }

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	void        SetName(char const* const szName) { m_name = szName; }
	char const* GetName() const                   { return m_name.c_str(); }
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

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

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> m_name;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
