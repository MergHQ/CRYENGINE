// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct IJoystickChannel;

namespace JoystickUtils
{
float Evaluate(IJoystickChannel* pChannel, float time);
void  SetKey(IJoystickChannel* pChannel, float time, float value, bool createIfMissing = true);
void  Serialize(IJoystickChannel* pChannel, XmlNodeRef node, bool bLoading);
bool  HasKey(IJoystickChannel* pChannel, float time);
void  RemoveKey(IJoystickChannel* pChannel, float time);
void  RemoveKeysInRange(IJoystickChannel* pChannel, float startTime, float endTime);
void  PlaceKey(IJoystickChannel* pChannel, float time);
}
