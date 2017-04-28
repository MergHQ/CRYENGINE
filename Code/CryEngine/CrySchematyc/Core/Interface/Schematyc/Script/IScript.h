// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/TimeValue.h>

#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Utils/Delegate.h"
#include "Schematyc/Utils/GUID.h"

namespace Schematyc
{
struct IScriptElement;

typedef CDelegate<EVisitStatus(IScriptElement&)> ScriptElementVisitor;

struct IScript
{
	virtual ~IScript() {}

	virtual SGUID             GetGUID() const = 0;

	virtual const char*       SetFilePath(const char* szFilePath) = 0;
	virtual const char*       GetFilePath() const = 0;

	virtual const CTimeValue& GetTimeStamp() const = 0;

	virtual void              SetRoot(IScriptElement* pRoot) = 0; // #SchematycTODO : Does this really need to be part of the public interface?
	virtual IScriptElement*   GetRoot() = 0;

	virtual EVisitStatus      VisitElements(const ScriptElementVisitor& visitor) = 0;

};
} // Schematyc
