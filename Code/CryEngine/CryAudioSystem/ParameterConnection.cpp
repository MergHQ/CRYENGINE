// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ParameterConnection.h"
#include "Common.h"
#include "Object.h"
#include "Common/IImpl.h"
#include "Common/IObject.h"

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CParameterConnection::~CParameterConnection()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during %s", __FUNCTION__);
	g_pIImpl->DestructParameter(m_pImplData);
}

//////////////////////////////////////////////////////////////////////////
void CParameterConnection::Set(CObject const& object, float const value) const
{
	object.GetImplDataPtr()->SetParameter(m_pImplData, value);
}

//////////////////////////////////////////////////////////////////////////
void CParameterConnection::SetGlobal(float const value) const
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during %s", __FUNCTION__);
	g_pIImpl->SetGlobalParameter(m_pImplData, value);
}
} // namespace CryAudio
