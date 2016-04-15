// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   StatsAgentPipe.h
//  Version:     v1.00
//  Created:     20/10/2011 by Sandy MacPherson
//  Description: Wrapper around platform-dependent pipe comms
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __STATS_AGENT_PIPES_H__
#define __STATS_AGENT_PIPES_H__

#pragma once

class CStatsAgentPipe
{
public:
	static void OpenPipe(const char *szPipeName);
	static void ClosePipe();

	static bool Send(const char *szMessage, const char *szPrefix = 0, const char *szDebugTag = 0);
	static const char* Receive();

	static bool PipeOpen();

private:
	CStatsAgentPipe(); // prevent instantiation

	static bool CreatePipe(const char *szName);
};

#endif // __STATS_AGENT_PIPES_H__
