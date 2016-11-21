// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Env/IEnvElement.h"

namespace Schematyc
{
// Forward declare structures.
struct SGUID;
// Forward declare classes.
class CAnyConstPtr;

struct IEnvSignal : public IEnvElementBase<EEnvElementType::Signal>
{
	virtual ~IEnvSignal() {}

	virtual uint32       GetInputCount() const = 0;
	virtual uint32       GetInputId(uint32 inputIdx) const = 0;
	virtual const char*  GetInputName(uint32 inputIdx) const = 0;
	virtual const char*  GetInputDescription(uint32 inputIdx) const = 0;
	virtual CAnyConstPtr GetInputData(uint32 inputIdx) const = 0;
};
} // Schematyc
