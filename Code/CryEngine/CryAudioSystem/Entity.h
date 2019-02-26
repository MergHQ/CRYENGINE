// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioSystem.h>

namespace CryAudio
{
template<typename IDType>
class CEntity
{
public:

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
	explicit CEntity(IDType const id, EDataScope const dataScope, char const* const szName)
		: m_id(id)
		, m_dataScope(dataScope)
		, m_name(szName)
	{}
#else
	explicit CEntity(IDType const id, EDataScope const dataScope)
		: m_id(id)
		, m_dataScope(dataScope)
	{}
#endif // CRY_AUDIO_USE_DEBUG_CODE

	CEntity() = delete;
	CEntity(CEntity const&) = delete;
	CEntity(CEntity&&) = delete;
	CEntity&   operator=(CEntity const&) = delete;
	CEntity&   operator=(CEntity&&) = delete;

	IDType     GetId() const        { return m_id; }
	EDataScope GetDataScope() const { return m_dataScope; }

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif // CRY_AUDIO_USE_DEBUG_CODE

protected:

	~CEntity() = default;

	IDType const     m_id;
	EDataScope const m_dataScope;

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
	CryFixedStringT<MaxControlNameLength> const m_name;
#endif // CRY_AUDIO_USE_DEBUG_CODE
};

using Control = CEntity<ControlId>;
} // namespace CryAudio
