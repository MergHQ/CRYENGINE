// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/PoolObject.h"
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
struct IEvent;
struct IObject;
struct ITrigger;
} // namespace Impl

class CTriggerConnection final : public CPoolObject<CTriggerConnection, stl::PSyncNone>
{
public:

	CTriggerConnection() = delete;
	CTriggerConnection(CTriggerConnection const&) = delete;
	CTriggerConnection(CTriggerConnection&&) = delete;
	CTriggerConnection& operator=(CTriggerConnection const&) = delete;
	CTriggerConnection& operator=(CTriggerConnection&&) = delete;

	explicit CTriggerConnection(
		TriggerImplId const triggerImplId,
		Impl::ITrigger const* const pImplData = nullptr)
		: m_triggerImplId(triggerImplId)
		, m_pImplData(pImplData)
	{}

	virtual ~CTriggerConnection();

	virtual ERequestStatus Execute(Impl::IObject* const pImplObject, Impl::IEvent* const pImplEvent) const;

	TriggerImplId const         m_triggerImplId;
	Impl::ITrigger const* const m_pImplData;
};
} // namespace CryAudio
