// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Env/IEnvElement.h"
#include "Schematyc/Utils/GUID.h"

namespace Schematyc
{

// Forward declare interfaces.
struct IObject;
struct IObjectPreviewer;
// Forward declare classes.
class CAnyConstPtr;

struct IEnvClass : public IEnvElementBase<EEnvElementType::Class>
{
	virtual ~IEnvClass() {}

	virtual uint32            GetInterfaceCount() const = 0;
	virtual SGUID             GetInterfaceGUID(uint32 interfaceIdx) const = 0;
	virtual uint32            GetComponentCount() const = 0;
	virtual SGUID             GetComponentTypeGUID(uint32 componentIdx) const = 0;
	virtual CAnyConstPtr      GetProperties() const = 0;
	virtual IObjectPreviewer* GetPreviewer() const = 0;
};

} // Schematyc
