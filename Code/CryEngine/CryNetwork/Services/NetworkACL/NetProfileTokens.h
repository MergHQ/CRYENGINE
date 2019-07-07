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

#ifndef __NET_PROFILE_TOKENS_H__
#define __NET_PROFILE_TOKENS_H__

#pragma once

#include <CryNetwork/INetwork.h>
#include <CryNetwork/INetworkService.h>

class CNetProfileTokens : public INetworkService, public INetProfileTokens
{
public:
	enum class EMode
	{
		ProfileTokens,
		Legacy
	};

private:
	typedef std::map<uint32, uint32> TTokenEntities;
	typedef std::map<uint32, SToken> TProfileTokens;

public:

	CNetProfileTokens();

	// INetworkService
	virtual ENetworkServiceState GetState() override                                                         { return m_state; }
	virtual void                 Close() override;
	virtual void                 CreateContextViewExtensions(bool server, IContextViewExtensionAdder* adder) override {};
	virtual IServerBrowser*      GetServerBrowser() override                                                 { return 0; }
	virtual INetworkChat*        GetNetworkChat() override                                                   { return 0; }
	virtual IServerReport*       GetServerReport() override                                                  { return 0; }
	virtual IStatsTrack*         GetStatsTrack(int version) override                                         { return 0; }
	virtual INatNeg*             GetNatNeg(INatNegListener* const) override                                  { return 0; }
	virtual INetworkProfile*     GetNetworkProfile() override                                                { return 0; }
	virtual ITestInterface*      GetTestInterface() override                                                 { return 0; }
	virtual IFileDownloader*     GetFileDownloader() override                                                { return 0; }
	virtual void                 GetMemoryStatistics(ICrySizer* pSizer) override                             {}
	virtual IPatchCheck*         GetPatchCheck() override                                                    { return 0; }
	virtual IHTTPGateway*        GetHTTPGateway() override                                                   { return 0; }
	virtual INetProfileTokens*   GetNetProfileTokens() override                                              { return static_cast<INetProfileTokens*>(this); }
	// ~INetworkService

	// INetProfileTokens
	virtual void Init() override;
	virtual void Server_AddTokens(const Array<SProfileTokenPair>& profileTokens) override;
	virtual void Client_SetToken(uint32 profile, const SToken& token) override;

	virtual void AddToken(uint32 profile, uint32 token) override;
	virtual bool IsValid(uint32 profile, uint32 token) override;
	// ~INetProfileTokens

	EMode GetMode() const { return m_mode; }
	
	bool Server_FindToken(uint32 profile, SProfileTokenPair& outProfileToken) const;
	void Client_GetToken(SProfileTokenPair& outTokenPair) const;

private:
	void ResetTokens();

private:
	ENetworkServiceState m_state;
	TTokenEntities       m_legacyTokens;

	TProfileTokens       m_profileTokensServer;

	SProfileTokenPair    m_tokenClient;

	EMode                m_mode;
	CryCriticalSectionNonRecursive m_lockTokens;
};

#endif // __NET_PROFILE_TOKENS_H__
