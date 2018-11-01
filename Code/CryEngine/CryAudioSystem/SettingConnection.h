// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/PoolObject.h"

namespace CryAudio
{
namespace Impl
{
struct ISetting;
} // namespace Impl

class CSettingConnection final : public CPoolObject<CSettingConnection, stl::PSyncNone>
{
public:

	CSettingConnection() = delete;
	CSettingConnection(CSettingConnection const&) = delete;
	CSettingConnection(CSettingConnection&&) = delete;
	CSettingConnection& operator=(CSettingConnection const&) = delete;
	CSettingConnection& operator=(CSettingConnection&&) = delete;

	explicit CSettingConnection(Impl::ISetting const* const pImplData)
		: m_pImplData(pImplData)
	{}

	~CSettingConnection();

	Impl::ISetting const* GetImplData() const { return m_pImplData; }

private:

	Impl::ISetting const* const m_pImplData;
};
} // namespace CryAudio
