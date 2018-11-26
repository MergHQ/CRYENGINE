// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEnvironmentConnection.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
class CEnvironment final : public IEnvironmentConnection
{
public:

	CEnvironment(CEnvironment const&) = delete;
	CEnvironment(CEnvironment&&) = delete;
	CEnvironment& operator=(CEnvironment const&) = delete;
	CEnvironment& operator=(CEnvironment&&) = delete;

	CEnvironment() = default;
	virtual ~CEnvironment() override = default;

	// IEnvironmentConnection
	virtual void Set(IObject* const pIObject, float const amount) override {}
	// ~IEnvironmentConnection
};
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
