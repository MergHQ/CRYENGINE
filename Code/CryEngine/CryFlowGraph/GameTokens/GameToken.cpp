// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   GameToken.cpp
//  Version:     v1.00
//  Created:     20/10/2005 by Timur.
//  Compilers:   Visual Studio.NET
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GameToken.h"
#include "GameTokenSystem.h"
#include "FlowSystem/FlowSerialize.h"

//////////////////////////////////////////////////////////////////////////
CGameTokenSystem* CGameToken::g_pGameTokenSystem = 0;

//////////////////////////////////////////////////////////////////////////
CGameToken::CGameToken()
	: m_changed(0.0f)
{
	m_nFlags = 0;
}

//////////////////////////////////////////////////////////////////////////
CGameToken::~CGameToken()
{
	Notify(EGAMETOKEN_EVENT_DELETE);
}

//////////////////////////////////////////////////////////////////////////
void CGameToken::Notify(EGameTokenEvent event)
{
	// Notify all listeners about game token event.
	for (Listeneres::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
	{
		(*it)->OnGameTokenEvent(event, this);
	}
}

//////////////////////////////////////////////////////////////////////////
void CGameToken::SetName(const char* sName)
{
	m_name = sName;
}

//////////////////////////////////////////////////////////////////////////
void CGameToken::SetType(EFlowDataTypes dataType)
{
	//@TODO: implement
	//m_value.SetType(dataType);
}

//////////////////////////////////////////////////////////////////////////
void CGameToken::SetValue(const TFlowInputData& val)
{
	if ((0 == (m_nFlags & EGAME_TOKEN_MODIFIED)) || val != m_value)
	{
		m_value = val;
		m_nFlags |= EGAME_TOKEN_MODIFIED;

		m_changed = gEnv->pTimer->GetFrameStartTime();
		g_pGameTokenSystem->Notify(EGAMETOKEN_EVENT_CHANGE, this);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CGameToken::GetValue(TFlowInputData& val) const
{
	val = m_value;
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CGameToken::SetValueAsString(const char* sValue, bool bDefault)
{
	if (bDefault)
	{
		SetFromString(m_value, sValue);
	}
	else
	{
		// if(strcmp(sValue,GetValueAsString()))
		{
			SetFromString(m_value, sValue);
			m_nFlags |= EGAME_TOKEN_MODIFIED;

			m_changed = gEnv->pTimer->GetFrameStartTime();
			g_pGameTokenSystem->Notify(EGAMETOKEN_EVENT_CHANGE, this);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
const char* CGameToken::GetValueAsString() const
{
	static string temp;
	temp = ConvertToString(m_value);
	return temp.c_str();
}

void CGameToken::GetMemoryStatistics(ICrySizer* s)
{
	s->AddObject(this, sizeof(*this));
	s->AddObject(m_name);
	s->AddObject(m_value);
	s->AddObject(m_listeners);
}
