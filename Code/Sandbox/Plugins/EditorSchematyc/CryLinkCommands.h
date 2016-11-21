// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma  once

struct IConsole;

namespace Schematyc
{
class CCryLinkCommands
{
public:

	void                     Register(IConsole* pConsole);
	void                     Unregister();

	static CCryLinkCommands& GetInstance();

private:

	CCryLinkCommands();

	~CCryLinkCommands();

private:

	IConsole*               m_pConsole;
	bool                    m_bRegistered;

	static CCryLinkCommands ms_instance;
};
} // Schematyc
