// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ITriggerConnection.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CBaseTriggerConnection : public ITriggerConnection
{
public:

	enum class EType : EnumFlagsType
	{
		Event,
		Snapshot,
	};

	CBaseTriggerConnection() = delete;
	CBaseTriggerConnection(CBaseTriggerConnection const&) = delete;
	CBaseTriggerConnection(CBaseTriggerConnection&&) = delete;
	CBaseTriggerConnection& operator=(CBaseTriggerConnection const&) = delete;
	CBaseTriggerConnection& operator=(CBaseTriggerConnection&&) = delete;

	virtual ~CBaseTriggerConnection() override = default;

	EType GetType() const { return m_type; }

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

protected:

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	explicit CBaseTriggerConnection(EType const type, char const* const szName)
		: m_type(type)
		, m_name(szName)
	{}
#else
	explicit CBaseTriggerConnection(EType const type)
		: m_type(type)
	{}
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	EType const m_type;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> const m_name;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
