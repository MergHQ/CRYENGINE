// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEnvironmentConnection.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
enum class EEnvironmentType : EnumFlagsType
{
	None,
	Bus,
	Parameter,
};

class CEnvironment : public IEnvironmentConnection
{
public:

	CEnvironment() = delete;
	CEnvironment(CEnvironment const&) = delete;
	CEnvironment(CEnvironment&&) = delete;
	CEnvironment& operator=(CEnvironment const&) = delete;
	CEnvironment& operator=(CEnvironment&&) = delete;

	virtual ~CEnvironment() override = default;

	// IEnvironmentConnection
	virtual void Set(IObject* const pIObject, float const amount) override;
	// ~IEnvironmentConnection

	EEnvironmentType GetType() const { return m_type; }

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	void        SetName(char const* const szName) { m_name = szName; }
	char const* GetName() const                   { return m_name.c_str(); }
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

protected:

	explicit CEnvironment(EEnvironmentType const type)
		: m_type(type)
	{}

	EEnvironmentType const m_type;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> m_name;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
