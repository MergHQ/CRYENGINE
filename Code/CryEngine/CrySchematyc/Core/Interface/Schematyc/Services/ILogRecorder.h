// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Services/ILog.h"
#include "Schematyc/Utils/Delegate.h"

namespace Schematyc
{
typedef CDelegate<EVisitStatus (const SLogMessageData&)> LogMessageVisitor;

struct ILogRecorder
{
	virtual ~ILogRecorder() {}

	virtual void Begin() = 0;
	virtual void End() = 0;
	virtual void VisitMessages(const LogMessageVisitor& visitor) = 0;
	virtual void Clear() = 0;
};
} // Schematyc
