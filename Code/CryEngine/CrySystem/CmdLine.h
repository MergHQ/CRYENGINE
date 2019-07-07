// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICmdLine.h>
#include "CmdLineArg.h"

class CCmdLine :
	public ICmdLine
{
public:
	CCmdLine(const char* commandLine);

	virtual const ICmdLineArg* GetArg(int n) const override;
	virtual int                GetArgCount() const override;
	virtual const ICmdLineArg* FindArg(const ECmdLineArgType ArgType, const char* name, bool caseSensitive = false) const override;

	const char*                GetCommandLine() const { return m_sCmdLine; }

private:
	void   PushCommand(const string& sCommand, const string& sParameter);
	string Next(char*& str);

	string                   m_sCmdLine;
	std::vector<CCmdLineArg> m_args;
};
