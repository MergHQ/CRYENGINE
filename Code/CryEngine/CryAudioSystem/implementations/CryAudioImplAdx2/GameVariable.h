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
class CGameVariable final : public IParameterConnection, public CPoolObject<CGameVariable, stl::PSyncNone>
{
public:

	CGameVariable() = delete;
	CGameVariable(CGameVariable const&) = delete;
	CGameVariable(CGameVariable&&) = delete;
	CGameVariable& operator=(CGameVariable const&) = delete;
	CGameVariable& operator=(CGameVariable&&) = delete;

	explicit CGameVariable(char const* const szName)
		: m_name(szName)
	{}

	virtual ~CGameVariable() override = default;

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
