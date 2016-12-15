// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Utils/Delegate.h"
#include "Schematyc/Utils/ScopedConnection.h"

namespace Schematyc
{
typedef CDelegate<void (TSerialize, int32, uint8, int)> NetworkSerializeCallback;

struct INetworkObject
{
	~INetworkObject() {}

	virtual bool   RegisterSerializeCallback(int32 aspects, const NetworkSerializeCallback& callback, CConnectionScope& connectionScope) = 0;
	virtual void   MarkAspectsDirty(int32 aspects) = 0;
	virtual uint16 GetChannelId() const = 0;
	virtual bool   ServerAuthority() const = 0;
	virtual bool   ClientAuthority() const = 0;
	virtual bool   LocalAuthority() const = 0;
};
} // Schematyc
