// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEnvironment.h>
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

class CEnvironment : public IEnvironment
{
public:

	CEnvironment() = delete;
	CEnvironment(CEnvironment const&) = delete;
	CEnvironment(CEnvironment&&) = delete;
	CEnvironment& operator=(CEnvironment const&) = delete;
	CEnvironment& operator=(CEnvironment&&) = delete;

	explicit CEnvironment(EEnvironmentType const type)
		: m_type(type)
	{}

	virtual ~CEnvironment() override = default;

	EEnvironmentType GetType() const { return m_type; }

private:

	EEnvironmentType const m_type;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
