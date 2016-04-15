// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#ifndef AISystemListener_h
	#define AISystemListener_h

struct IPuppet;

class IAISystemListener
{
public:
	virtual ~IAISystemListener(){}
	//////////////////////////////////////////////////////////////////////////////////////
	// I'd like to get rid of this. There were in fact no references to it in Crysis 2.

	enum EAISystemEvent
	{
		eAISE_Reset,
	};

	virtual void OnEvent(EAISystemEvent event) {}

	//////////////////////////////////////////////////////////////////////////////////////

	virtual void OnAgentDeath(EntityId deadEntityID, EntityId killerID) {}
	virtual void OnAgentUpdate(EntityId entityID)                       {}
};

#endif // AISystemListener_h
