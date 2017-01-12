// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Env/Elements/IEnvInterface.h"
#include "Schematyc/Network/INetworkSpawnParams.h"
#include "Schematyc/Utils/Delegate.h"
#include "Schematyc/Utils/EnumFlags.h"
#include "Schematyc/Utils/GUID.h"
#include "Schematyc/Utils/TypeUtils.h"

namespace Schematyc
{

// Forward declare interfaces.
struct IEnvAction;
struct IEnvClass;
struct IEnvComponent;
struct IEnvDataType;
struct IEnvFunction;
struct IEnvModule;
struct IEnvPackage;
struct IEnvRegistrar;
struct IEnvSignal;

typedef std::unique_ptr<IEnvPackage>                          IEnvPackagePtr;

typedef CDelegate<EVisitStatus(const IEnvPackage&)>           EnvPackageConstVisitor;
typedef CDelegate<EVisitStatus(const IEnvModule&)>            EnvModuleConstVisitor;
typedef CDelegate<EVisitStatus(const IEnvDataType&)>          EnvDataTypeConstVisitor;
typedef CDelegate<EVisitStatus(const IEnvSignal&)>            EnvSignalConstVisitor;
typedef CDelegate<EVisitStatus(const IEnvFunction&)>          EnvFunctionConstVisitor;
typedef CDelegate<EVisitStatus(const IEnvClass&)>             EnvClassConstVisitor;
typedef CDelegate<EVisitStatus(const IEnvInterface&)>         EnvInterfaceConstVisitor;
typedef CDelegate<EVisitStatus(const IEnvInterfaceFunction&)> EnvInterfaceFunctionConstVisitor;
typedef CDelegate<EVisitStatus(const IEnvComponent&)>         EnvComponentConstVisitor;
typedef CDelegate<EVisitStatus(const IEnvAction&)>            EnvActionConstVisitor;

struct IEnvRegistry
{
	virtual ~IEnvRegistry() {}

	virtual bool                         RegisterPackage(IEnvPackagePtr&& pPackage) = 0;
	virtual const IEnvPackage*           GetPackage(const SGUID& guid) const = 0;
	virtual void                         VisitPackages(const EnvPackageConstVisitor& visitor) const = 0;

	virtual const IEnvElement&           GetRoot() const = 0;
	virtual const IEnvElement*           GetElement(const SGUID& guid) const = 0;

	virtual const IEnvModule*            GetModule(const SGUID& guid) const = 0;
	virtual void                         VisitModules(const EnvModuleConstVisitor& visitor) const = 0;

	virtual const IEnvDataType*          GetDataType(const SGUID& guid) const = 0;
	virtual void                         VisitDataTypes(const EnvDataTypeConstVisitor& visitor) const = 0;

	virtual const IEnvSignal*            GetSignal(const SGUID& guid) const = 0;
	virtual void                         VisitSignals(const EnvSignalConstVisitor& visitor) const = 0;

	virtual const IEnvFunction*          GetFunction(const SGUID& guid) const = 0;
	virtual void                         VisitFunctions(const EnvFunctionConstVisitor& visitor) const = 0;

	virtual const IEnvClass*             GetClass(const SGUID& guid) const = 0;
	virtual void                         VisitClasses(const EnvClassConstVisitor& visitor) const = 0;

	virtual const IEnvInterface*         GetInterface(const SGUID& guid) const = 0;
	virtual void                         VisitInterfaces(const EnvInterfaceConstVisitor& visitor) const = 0;

	virtual const IEnvInterfaceFunction* GetInterfaceFunction(const SGUID& guid) const = 0;
	virtual void                         VisitInterfaceFunctions(const EnvInterfaceFunctionConstVisitor& visitor) const = 0;

	virtual const IEnvComponent*         GetComponent(const SGUID& guid) const = 0;
	virtual void                         VisitComponents(const EnvComponentConstVisitor& visitor) const = 0;

	virtual const IEnvAction*            GetAction(const SGUID& guid) const = 0;
	virtual void                         VisitActions(const EnvActionConstVisitor& visitor) const = 0;

	virtual void                         BlacklistElement(const SGUID& guid) = 0;
	virtual bool                         IsBlacklistedElement(const SGUID& guid) const = 0;
};

} // Schematyc
