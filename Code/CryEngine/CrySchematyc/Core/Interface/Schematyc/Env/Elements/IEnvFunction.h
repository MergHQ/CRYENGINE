// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Env/IEnvElement.h"
#include "Schematyc/Utils/EnumFlags.h"
#include "Schematyc/Utils/GUID.h"

namespace Schematyc
{
// Forward declare structures.
struct SGUID;
struct SRuntimeContext;
// Forward declare classes.
class CAnyConstPtr;
class CCommonTypeInfo;
class CTypeName;

enum class EEnvFunctionFlags
{
	None         = 0,
	Member       = BIT(0), // Function is member of object.
	Pure         = BIT(1), // Function does not affect global state.
	Const        = BIT(2), // Function is constant.
	Construction = BIT(3)  // Function can be called from construction graph.
};

typedef CEnumFlags<EEnvFunctionFlags> EnvFunctionFlags;

struct IEnvFunction : public IEnvElementBase<EEnvElementType::Function>
{
	virtual ~IEnvFunction() {}

	virtual bool                   Validate() const = 0;
	virtual EnvFunctionFlags       GetFlags() const = 0;
	virtual const CCommonTypeInfo* GetObjectTypeInfo() const = 0;
	virtual uint32                 GetInputCount() const = 0;
	virtual uint32                 GetInputId(uint32 inputIdx) const = 0;
	virtual const char*            GetInputName(uint32 inputIdx) const = 0;
	virtual const char*            GetInputDescription(uint32 inputIdx) const = 0;
	virtual CAnyConstPtr           GetInputData(uint32 inputIdx) const = 0;
	virtual uint32                 GetOutputCount() const = 0;
	virtual uint32                 GetOutputId(uint32 outputIdx) const = 0;
	virtual const char*            GetOutputName(uint32 outputIdx) const = 0;
	virtual const char*            GetOutputDescription(uint32 outputIdx) const = 0;
	virtual CAnyConstPtr           GetOutputData(uint32 outputIdx) const = 0;
	virtual void                   Execute(SRuntimeContext& context, void* pObject) const = 0; // #SchematycTODO : Can we return some kind of function pointer / proxy object here to avoid virtual function at runtime?
};

DECLARE_SHARED_POINTERS(IEnvFunction)
} // Schematyc
