// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "GameVariableState.h"

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	#include "BaseObject.h"
	#include <Logger.h>
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CGameVariableState::Set(IObject* const pIObject)
{
	SetGlobally();

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);
	Cry::Audio::Log(ELogType::Warning, R"(Adx2 - GameVariable "%s" was set to %f on object "%s". Consider setting it globally.)",
	                m_name.c_str(), static_cast<float>(m_value), pBaseObject->GetName());
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CGameVariableState::SetGlobally()
{
	criAtomEx_SetGameVariableByName(static_cast<CriChar8 const*>(m_name.c_str()), m_value);

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	g_gameVariableValues[m_name] = m_value;
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
