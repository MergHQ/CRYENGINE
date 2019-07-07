// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id$
   $DateTime$
   Description: keep list of profile and its token ids
   profile provides its token id during connection
   server uses this server to validate token per profile
   -------------------------------------------------------------------------
   History:
   - 29/09/2009 : Created by Sergii Rustamov
*************************************************************************/

#include "StdAfx.h"
#include "NetProfileTokens.h"

CNetProfileTokens::CNetProfileTokens()
	: m_state(eNSS_Initializing)
	, m_mode(EMode::Legacy)
{
}

void CNetProfileTokens::Init()
{
	CryAutoLock<CryCriticalSectionNonRecursive> lock(m_lockTokens);
	ResetTokens();
	m_state = eNSS_Ok;
}

void CNetProfileTokens::Close()
{
	CryAutoLock<CryCriticalSectionNonRecursive> lock(m_lockTokens);
	ResetTokens();
	m_state = eNSS_Closed;
}

void CNetProfileTokens::Server_AddTokens(const Array<SProfileTokenPair>& profileTokens)
{
	CryAutoLock<CryCriticalSectionNonRecursive> lock(m_lockTokens);
	m_mode = EMode::ProfileTokens;
	for (const SProfileTokenPair& pt : profileTokens)
	{
		m_profileTokensServer[pt.profile] = pt.token;
	}
}

void CNetProfileTokens::Client_SetToken(uint32 profile, const SToken& token)
{
	CryAutoLock<CryCriticalSectionNonRecursive> lock(m_lockTokens);
	m_mode = EMode::ProfileTokens;
	m_tokenClient.profile = profile;
	m_tokenClient.token = token;
}


void CNetProfileTokens::AddToken(uint32 profile, uint32 token)
{
	CryAutoLock<CryCriticalSectionNonRecursive> lock(m_lockTokens);
	if (m_mode == EMode::Legacy)
	{
		m_legacyTokens.insert(std::make_pair(profile, token));
	}
	else
	{
		NetWarning("NetProfileTokens: can't add legacy token, wrong mode");
	}
}

bool CNetProfileTokens::IsValid(uint32 profile, uint32 token)
{
	CryAutoLock<CryCriticalSectionNonRecursive> lock(m_lockTokens);

	if (m_mode == EMode::Legacy)
	{
		TTokenEntities::iterator iter = m_legacyTokens.find(profile);
		if (m_legacyTokens.end() != iter)
		{
			return (m_legacyTokens[profile] == token);
		}
	}
	else
	{
		NetWarning("NetProfileTokens: can't validate legacy token, wrong mode");
	}
	return false;
}

bool CNetProfileTokens::Server_FindToken(uint32 profile, SProfileTokenPair& outProfileToken) const
{
	CryAutoLock<CryCriticalSectionNonRecursive> lock(m_lockTokens);

	TProfileTokens::const_iterator iter = m_profileTokensServer.find(profile);
	if (iter != m_profileTokensServer.end())
	{
		outProfileToken.profile = profile;
		outProfileToken.token = iter->second;
		return true;
	}
	return false;
}

void CNetProfileTokens::Client_GetToken(SProfileTokenPair& outTokenPair) const
{
	CryAutoLock<CryCriticalSectionNonRecursive> lock(m_lockTokens);
	outTokenPair = m_tokenClient;
}

void CNetProfileTokens::ResetTokens()
{
	// Switch service into legacy mode by default for API backwards compatibility.
	// It's upgraded to EMode::ProfileTokens mode on first use of the new API and cannot be downgraded.
	m_mode = EMode::Legacy;
	m_legacyTokens.clear();
	m_profileTokensServer.clear();
	m_tokenClient = {};
}

