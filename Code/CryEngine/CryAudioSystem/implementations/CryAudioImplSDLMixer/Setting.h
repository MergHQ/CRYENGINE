// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ISettingConnection.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
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
	virtual void Load() const override   {}
	virtual void Unload() const override {}
	// ~ISettingConnection
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio