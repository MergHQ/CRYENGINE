// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ISwitchStateConnection.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CSelectorLabel final : public ISwitchStateConnection, public CPoolObject<CSelectorLabel, stl::PSyncNone>
{
public:

	CSelectorLabel() = delete;
	CSelectorLabel(CSelectorLabel const&) = delete;
	CSelectorLabel(CSelectorLabel&&) = delete;
	CSelectorLabel& operator=(CSelectorLabel const&) = delete;
	CSelectorLabel& operator=(CSelectorLabel&&) = delete;

	explicit CSelectorLabel(char const* const szSelectorName, char const* const szSelectorLabelName)
		: m_selectorName(szSelectorName)
		, m_selectorLabelName(szSelectorLabelName)
	{}

	virtual ~CSelectorLabel() override = default;

	// ISwitchStateConnection
	virtual void Set(IObject* const pIObject) override;
	virtual void SetGlobally() override;
	// ~ISwitchStateConnection

private:

	CryFixedStringT<MaxControlNameLength> const m_selectorName;
	CryFixedStringT<MaxControlNameLength> const m_selectorLabelName;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
