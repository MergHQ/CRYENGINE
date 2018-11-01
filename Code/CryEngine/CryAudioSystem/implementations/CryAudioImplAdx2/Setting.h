// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ISetting.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CSetting final : public ISetting, public CPoolObject<CSetting, stl::PSyncNone>
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

	// ISetting
	virtual void Load() const override;
	virtual void Unload() const override;
	// ~ISetting

private:

	CryFixedStringT<MaxControlNameLength> const m_name;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
