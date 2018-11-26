// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ISettingConnection.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CSetting final : public ISettingConnection
{
public:

	CSetting(CSetting const&) = delete;
	CSetting(CSetting&&) = delete;
	CSetting& operator=(CSetting const&) = delete;
	CSetting& operator=(CSetting&&) = delete;

	CSetting() = default;
	virtual ~CSetting() override = default;

	// ISettingConnection
	virtual void Load() override   {}
	virtual void Unload() override {}
	// ~ISettingConnection
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
