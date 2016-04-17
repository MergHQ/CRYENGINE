// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <vector>
#include <memory>

namespace ACE
{
enum EACEControlType
{
	eACEControlType_Trigger = 0,
	eACEControlType_RTPC,
	eACEControlType_Switch,
	eACEControlType_State,
	eACEControlType_Environment,
	eACEControlType_Preload,
	eACEControlType_NumTypes
};

typedef unsigned int ItemType;
static const ItemType AUDIO_SYSTEM_INVALID_TYPE = 0;

typedef unsigned int CID;
static const CID ACE_INVALID_ID = 0;

typedef std::vector<CID> ControlList;

class IAudioConnection;
typedef std::shared_ptr<IAudioConnection> ConnectionPtr;
}
