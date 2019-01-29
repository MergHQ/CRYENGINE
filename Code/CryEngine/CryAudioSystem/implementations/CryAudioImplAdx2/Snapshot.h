// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ITriggerConnection.h>
#include <PoolObject.h>

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
constexpr char const* g_debugNoneSnapshot = "<none>";
extern CryFixedStringT<MaxControlNameLength> g_debugActiveSnapShotName;
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

class CSnapshot final : public ITriggerConnection, public CPoolObject<CSnapshot, stl::PSyncNone>
{
public:

	enum class EActionType : EnumFlagsType
	{
		None,
		Start,
		Stop,
	};

	CSnapshot() = delete;
	CSnapshot(CSnapshot const&) = delete;
	CSnapshot(CSnapshot&&) = delete;
	CSnapshot& operator=(CSnapshot const&) = delete;
	CSnapshot& operator=(CSnapshot&&) = delete;

	explicit CSnapshot(
		char const* const szName,
		EActionType const type,
		CriSint32 const changeoverTime)
		: m_name(szName)
		, m_actionType(type)
		, m_changeoverTime(changeoverTime)
	{}

	virtual ~CSnapshot() override = default;

	// CryAudio::Impl::ITriggerConnection
	virtual ETriggerResult Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId) override;
	virtual void           Stop(IObject* const pIObject) override;
	virtual ERequestStatus Load()  const override                                                { return ERequestStatus::Success; }
	virtual ERequestStatus Unload() const override                                               { return ERequestStatus::Success; }
	virtual ERequestStatus LoadAsync(TriggerInstanceId const triggerInstanceId) const override   { return ERequestStatus::Success; }
	virtual ERequestStatus UnloadAsync(TriggerInstanceId const triggerInstanceId) const override { return ERequestStatus::Success; }
	// ~CryAudio::Impl::ITriggerConnection

private:

	CryFixedStringT<MaxControlNameLength> const m_name;
	EActionType const                           m_actionType;
	CriSint32 const                             m_changeoverTime;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
