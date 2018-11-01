// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Entity.h"
#include "Common.h"
#include "Common/PoolObject.h"

namespace CryAudio
{
class CObject;

class CParameter final : public Control, public CPoolObject<CParameter, stl::PSyncNone>
{
public:

	CParameter() = delete;
	CParameter(CParameter const&) = delete;
	CParameter(CParameter&&) = delete;
	CParameter& operator=(CParameter const&) = delete;
	CParameter& operator=(CParameter&&) = delete;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	explicit CParameter(
		ControlId const id,
		EDataScope const dataScope,
		ParameterConnections const& connections,
		char const* const szName)
		: Control(id, dataScope, szName)
		, m_connections(connections)
	{}
#else
	explicit CParameter(
		ControlId const id,
		EDataScope const dataScope,
		ParameterConnections const& connections)
		: Control(id, dataScope)
		, m_connections(connections)
	{}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	~CParameter();

	void Set(CObject const& object, float const value) const;

private:

	ParameterConnections const m_connections;
};
} // namespace CryAudio
