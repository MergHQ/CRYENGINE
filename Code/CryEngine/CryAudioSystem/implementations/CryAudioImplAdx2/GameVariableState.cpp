// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "GameVariableState.h"

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	#include "Common.h"
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
	criAtomEx_SetGameVariableByName(static_cast<CriChar8 const*>(m_name), m_value);

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	g_gameVariableValues[m_name] = m_value;
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CGameVariableState::SetGlobally()
{
	Set(nullptr);
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
