// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "SwitchStateConnection.h"
#include "Common.h"
#include "Object.h"
#include "Common/IImpl.h"
#include "Common/IObject.h"

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CSwitchStateConnection::~CSwitchStateConnection()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during %s", __FUNCTION__);
	g_pIImpl->DestructSwitchState(m_pImplData);
}

//////////////////////////////////////////////////////////////////////////
void CSwitchStateConnection::Set(CObject const& object) const
{
	object.GetImplDataPtr()->SetSwitchState(m_pImplData);
}

//////////////////////////////////////////////////////////////////////////
void CSwitchStateConnection::SetGlobal() const
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during %s", __FUNCTION__);
	g_pIImpl->SetGlobalSwitchState(m_pImplData);
}
} // namespace CryAudio
