// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Env/EnvElementBase.h>
#include <Schematyc/Env/IEnvPackage.h>
#include <Schematyc/Env/IEnvRegistrar.h>
#include <Schematyc/Env/IEnvRegistry.h>

namespace Schematyc
{
// Forward declare interfaces.
struct IEnvAction;
struct IEnvClass;
struct IEnvComponent;
struct IEnvDataType;
struct IEnvElement;
struct IEnvFunction;
struct IEnvInterface;
struct IEnvInterfaceFunction;
struct IEnvModule;
struct IEnvSignal;
// Forward declare structures.
struct SEnvPackageElement;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(IEnvAction)
DECLARE_SHARED_POINTERS(IEnvClass)
DECLARE_SHARED_POINTERS(IEnvComponent)
DECLARE_SHARED_POINTERS(IEnvDataType)
DECLARE_SHARED_POINTERS(IEnvElement)
DECLARE_SHARED_POINTERS(IEnvFunction)
DECLARE_SHARED_POINTERS(IEnvInterface)
DECLARE_SHARED_POINTERS(IEnvInterfaceFunction)
DECLARE_SHARED_POINTERS(IEnvModule)
DECLARE_SHARED_POINTERS(IEnvSignal)

typedef std::vector<SEnvPackageElement> EnvPackageElements;

struct IEnvRoot : public IEnvElementBase<EEnvElementType::Root>
{
	virtual ~IEnvRoot() {}
};

class CEnvRoot : public CEnvElementBase<IEnvRoot>
{
public:

	CEnvRoot();

	// IEnvElement
	virtual bool IsValidScope(IEnvElement& scope) const override;
	// ~IEnvElement
};

class CEnvRegistry : public IEnvRegistry
{
private:

	typedef std::unordered_map<SGUID, IEnvPackagePtr>                Packages;
	typedef std::unordered_map<SGUID, IEnvElementPtr>                Elements;
	typedef std::unordered_map<SGUID, IEnvModulePtr>                 Modules;
	typedef std::unordered_map<SGUID, IEnvDataTypePtr>               DataTypes;
	typedef std::unordered_map<SGUID, IEnvSignalPtr>                 Signals;
	typedef std::unordered_map<SGUID, IEnvFunctionPtr>               Functions;
	typedef std::unordered_map<SGUID, IEnvClassPtr>                  Classes;
	typedef std::unordered_map<SGUID, IEnvInterfacePtr>              Interfaces;
	typedef std::unordered_map<SGUID, IEnvInterfaceFunctionConstPtr> InterfaceFunctions;
	typedef std::unordered_map<SGUID, IEnvComponentPtr>              Components;
	typedef std::unordered_map<SGUID, IEnvActionPtr>                 Actions;
	typedef std::unordered_set<SGUID>                                Blacklist;

public:

	// IEnvRegistry

	virtual bool                         RegisterPackage(IEnvPackagePtr&& pPackage) override;
	virtual const IEnvPackage*           GetPackage(const SGUID& guid) const override;
	virtual void                         VisitPackages(const EnvPackageConstVisitor& visitor) const override;

	virtual const IEnvElement&           GetRoot() const override;
	virtual const IEnvElement*           GetElement(const SGUID& guid) const override;

	virtual const IEnvModule*            GetModule(const SGUID& guid) const override;
	virtual void                         VisitModules(const EnvModuleConstVisitor& visitor) const override;

	virtual const IEnvDataType*          GetDataType(const SGUID& guid) const override;
	virtual void                         VisitDataTypes(const EnvDataTypeConstVisitor& visitor) const override;

	virtual const IEnvSignal*            GetSignal(const SGUID& guid) const override;
	virtual void                         VisitSignals(const EnvSignalConstVisitor& visitor) const override;

	virtual const IEnvFunction*          GetFunction(const SGUID& guid) const override;
	virtual void                         VisitFunctions(const EnvFunctionConstVisitor& visitor) const override;

	virtual const IEnvClass*             GetClass(const SGUID& guid) const override;
	virtual void                         VisitClasses(const EnvClassConstVisitor& visitor) const override;

	virtual const IEnvInterface*         GetInterface(const SGUID& guid) const override;
	virtual void                         VisitInterfaces(const EnvInterfaceConstVisitor& visitor) const override;

	virtual const IEnvInterfaceFunction* GetInterfaceFunction(const SGUID& guid) const override;
	virtual void                         VisitInterfaceFunctions(const EnvInterfaceFunctionConstVisitor& visitor) const override;

	virtual const IEnvComponent*         GetComponent(const SGUID& guid) const override;
	virtual void                         VisitComponents(const EnvComponentConstVisitor& visitor) const override;

	virtual const IEnvAction*            GetAction(const SGUID& guid) const override;
	virtual void                         VisitActions(const EnvActionConstVisitor& visitor) const override;

	virtual void                         BlacklistElement(const SGUID& guid) override;
	virtual bool                         IsBlacklistedElement(const SGUID& guid) const override;

	// ~IEnvRegistry

	void Refresh();

private:

	bool         RegisterPackageElements(const EnvPackageElements& packageElements);
	void         ReleasePackageElements(const EnvPackageElements& packageElements);

	bool         ValidateComponentDependencies() const;

	IEnvElement* GetElement(const SGUID& guid);

private:

	Packages           m_packages;

	CEnvRoot           m_root;
	Elements           m_elements;

	Modules            m_modules;
	DataTypes          m_dataTypes;
	Signals            m_signals;
	Functions          m_functions;
	Classes            m_classes;
	Interfaces         m_interfaces;
	InterfaceFunctions m_interfaceFunctions;
	Components         m_components;
	Actions            m_actions;

	Blacklist          m_blacklist;
};
} // Schematyc
