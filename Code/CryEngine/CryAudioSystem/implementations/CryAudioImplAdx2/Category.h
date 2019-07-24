// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IParameterConnection.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CCategory final : public IParameterConnection, public CPoolObject<CCategory, stl::PSyncNone>
{
public:

	CCategory() = delete;
	CCategory(CCategory const&) = delete;
	CCategory(CCategory&&) = delete;
	CCategory& operator=(CCategory const&) = delete;
	CCategory& operator=(CCategory&&) = delete;

	explicit CCategory(char const* const szName)
		: m_name(szName)
	{}

	virtual ~CCategory() override = default;

	// IParameterConnection
	virtual void Set(IObject* const pIObject, float const value) override;
	virtual void SetGlobally(float const value) override;
	// ~IParameterConnection

private:

	CryFixedStringT<MaxControlNameLength> const m_name;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
