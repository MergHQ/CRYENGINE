// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/PoolObject.h"

namespace CryAudio
{
class CObject;

namespace Impl
{
struct IParameter;
} // namespace Impl

class CParameterConnection final : public CPoolObject<CParameterConnection, stl::PSyncNone>
{
public:

	CParameterConnection() = delete;
	CParameterConnection(CParameterConnection const&) = delete;
	CParameterConnection(CParameterConnection&&) = delete;
	CParameterConnection& operator=(CParameterConnection const&) = delete;
	CParameterConnection& operator=(CParameterConnection&&) = delete;

	explicit CParameterConnection(Impl::IParameter const* const pImplData)
		: m_pImplData(pImplData)
	{}

	virtual ~CParameterConnection();

	void Set(CObject const& object, float const value) const;

private:

	Impl::IParameter const* const m_pImplData = nullptr;
};
} // namespace CryAudio
