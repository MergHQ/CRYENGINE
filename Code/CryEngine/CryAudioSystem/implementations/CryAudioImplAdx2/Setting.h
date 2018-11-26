// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ISettingConnection.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CSetting final : public ISettingConnection, public CPoolObject<CSetting, stl::PSyncNone>
{
public:

	CSetting() = delete;
	CSetting(CSetting const&) = delete;
	CSetting(CSetting&&) = delete;
	CSetting& operator=(CSetting const&) = delete;
	CSetting& operator=(CSetting&&) = delete;

	explicit CSetting(char const* const szName)
		: m_name(szName)
	{}

	virtual ~CSetting() override = default;

	// ISettingConnection
	virtual void Load() override;
	virtual void Unload() override;
	// ~ISettingConnection

private:

	CryFixedStringT<MaxControlNameLength> const m_name;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
