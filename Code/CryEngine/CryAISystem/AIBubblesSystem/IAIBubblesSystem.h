// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   IAIBubblesSystem.h
//  Version:     v1.00
//  Created:     30/7/2009 by Ivo Frey
//  Compilers:   Visual Studio.NET
//  Description: Bubbles System interface
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __IAIBubblesSystem_h__
#define __IAIBubblesSystem_h__

#if _MSC_VER > 1000
	#pragma once
#endif

#define MAX_BUBBLE_MESSAGE_LENGHT 512
#define MIN_CHARACTERS_INTERVAL   30

#include <CryEntitySystem/IEntity.h>

typedef uint32 TBubbleRequestOptionFlags;

struct SAIBubbleRequest
{
	enum ERequestType
	{
		eRT_ErrorMessage = 0,
		eRT_PrototypeDialog,
	};

	SAIBubbleRequest(const EntityId _entityID, const char* _message, TBubbleRequestOptionFlags _flags, float _duration = 0.0f, const ERequestType _requestType = eRT_ErrorMessage)
		: entityID(_entityID)
		, message(_message)
		, flags(_flags)
		, duration(_duration)
		, requestType(_requestType)
		, totalLinesFormattedMessage(0)
	{
		FormatMessageWithMultiline(MIN_CHARACTERS_INTERVAL);
	}

	SAIBubbleRequest(const SAIBubbleRequest& request)
		: entityID(request.entityID)
		, message(request.message)
		, flags(request.flags)
		, duration(request.duration)
		, requestType(request.requestType)
		, totalLinesFormattedMessage(request.totalLinesFormattedMessage)
	{}

	size_t GetFormattedMessage(stack_string& outMessage) const { outMessage = message; return totalLinesFormattedMessage; }
	void   GetMessage(stack_string& outMessage) const
	{
		outMessage = message;
		std::replace(outMessage.begin(), outMessage.end(), '\n', ' ');
	}

	const EntityId            GetEntityID() const    { return entityID; }
	TBubbleRequestOptionFlags GetFlags() const       { return flags; }
	float                     GetDuration() const    { return duration; }
	ERequestType              GetRequestType() const { return requestType; }

private:
	void FormatMessageWithMultiline(size_t minimumCharactersInterval)
	{
		size_t pos = message.find(' ', minimumCharactersInterval);
		size_t i = 1;
		while (pos != message.npos)
		{
			message.replace(pos, 1, "\n");
			pos = message.find(' ', minimumCharactersInterval * (++i));
		}
		totalLinesFormattedMessage = i;
	}

	EntityId                                   entityID;
	CryFixedStringT<MAX_BUBBLE_MESSAGE_LENGHT> message;
	TBubbleRequestOptionFlags                  flags;
	float duration;
	const ERequestType                         requestType;
	size_t totalLinesFormattedMessage;

};

bool AIQueueBubbleMessage(const char* messageName, const EntityId entityID,
                          const char* message, uint32 flags, float duration = 0, SAIBubbleRequest::ERequestType requestType = SAIBubbleRequest::eRT_ErrorMessage);

enum EBubbleRequestOption
{
	eBNS_None          = 0,
	eBNS_Log           = BIT(0),
	eBNS_Balloon       = BIT(1),
	eBNS_BlockingPopup = BIT(2),
	eBNS_LogWarning    = BIT(3),
};

struct IAIBubblesSystem
{
	virtual ~IAIBubblesSystem() {};

	virtual void Init() = 0;
	/*
	   messageName is an identifier for the part of the code where the
	   QueueMessage is called to avoid queuing the same message
	   several time before the previous message is actually expired
	   inside the BubbleSystem
	 */
	virtual void QueueMessage(const char* messageName, const SAIBubbleRequest& request) = 0;
	virtual void Update() = 0;
	virtual void Reset() = 0;
};

#endif // __IAIBubblesSystem_h__
