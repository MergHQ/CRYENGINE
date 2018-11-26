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

protected:

	explicit CEnvironment(EEnvironmentType const type)
		: m_type(type)
	{}

	EEnvironmentType const m_type;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
