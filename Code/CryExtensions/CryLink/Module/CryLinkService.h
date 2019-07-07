// Copyright 2001-2019 Crytek GmbH. All rights reserved.

#pragma once

#include <CryLink/IFramework.h>
#include <CrySystem/IConsole.h>

namespace CryLinkService
{
class CCryLinkService : public ICryLinkService, public IHttpServerListener, public IOutputPrintSink
{
	static const char s_szCryLinkProtocolId[];

	enum { noConnectionID = -1 };

	enum class EConnectionState : uint16
	{
		Rejected = 0,
		Connected,
		ChallengeSent,
		Authenticated,
	};

	struct SCommandInfo
	{
		string command;
		int32  connectionId;

		SCommandInfo(const char* _szCommand, int32 _connectionId)
			: command(_szCommand)
			, connectionId(_connectionId)
		{}
	};
	typedef std::deque<SCommandInfo> CommandsQueue;

	struct SConnectionInfo
	{
		string           clientName;
		string           challange;
		int32            connectionId;
		EConnectionState state;

		SConnectionInfo(const char* _szClientName, int32 _connectionId)
			: clientName(_szClientName)
			, state(EConnectionState::Connected)
			, connectionId(_connectionId)
		{}
	};
	typedef std::map<int32 /* connectionId */, SConnectionInfo> ConnectionsById;

public:
	CCryLinkService();
	~CCryLinkService();

	// ICryLinkService
	virtual bool          Start(ISimpleHttpServer& httpServer) override;
	virtual void          Stop() override;
	virtual void          Pause(bool bPause) override;

	virtual EServiceState GetServiceState() const override { return m_serviceState; }

	virtual void          Update() override;
	// ~ICryLinkService

protected:
	bool SendChallenge(const string& command, SConnectionInfo& connectionInfo);
	bool Authenticate(const string& command, SConnectionInfo& connectionInfo);
	bool ExecuteCommand(const string& command, SConnectionInfo& connectionInfo);

	// IHttpServerListener
	virtual void OnStartResult(bool started, EResultDesc desc) override {}

	virtual void OnClientConnected(int connectionID, string client) override;
	virtual void OnClientDisconnected(int connectionID) override;

	virtual void OnGetRequest(int connectionID, string url) override;
	virtual void OnRpcRequest(int connectionID, string xml) override;
	// ~IHttpServerListener

	// IOutputPrintSink
	virtual void Print(const char* szText) override;
	// ~IOutputPrintSink

	bool             IsClientConnected(int32 connectionId) const;
	SConnectionInfo* GetConnectionInfo(int32 connectionId);

	static void      LogMessage(const char* szFormat, ...);
	static void      LogWarning(const char* szFormat, ...);

private:
	ISystem*           m_pSystem;
	ISimpleHttpServer* m_pHttpServer;
	CommandsQueue      m_commandQueue;
	ConnectionsById    m_connections;

	string             m_consoleOutput;
	string             m_password;

	EServiceState      m_serviceState;
};
}
