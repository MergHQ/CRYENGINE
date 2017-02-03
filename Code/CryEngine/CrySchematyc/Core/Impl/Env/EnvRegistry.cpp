// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EnvRegistry.h"

#include <CrySerialization/IArchiveHost.h>
#include <Schematyc/Env/Elements/IEnvAction.h>
#include <Schematyc/Env/Elements/IEnvClass.h>
#include <Schematyc/Env/Elements/IEnvComponent.h>
#include <Schematyc/Env/Elements/IEnvDataType.h>
#include <Schematyc/Env/Elements/IEnvInterface.h>
#include <Schematyc/Env/Elements/IEnvFunction.h>
#include <Schematyc/Env/Elements/IEnvModule.h>
#include <Schematyc/Env/Elements/IEnvSignal.h>
#include <Schematyc/Reflection/ComponentDesc.h>
#include <Schematyc/Utils/Assert.h>
#include <Schematyc/Utils/StackString.h>

namespace Schematyc
{
struct SEnvPackageElement
{
	inline SEnvPackageElement(const SGUID& _elementGUID, const IEnvElementPtr& _pElement, const SGUID& _scopeGUID)
		: elementGUID(_elementGUID)
		, pElement(_pElement)
		, scopeGUID(_scopeGUID)
	{}

	SGUID          elementGUID;
	IEnvElementPtr pElement;
	SGUID          scopeGUID;
};

class CEnvPackageElementRegistrar : public IEnvElementRegistrar, public IEnvRegistrar
{
public:

	inline CEnvPackageElementRegistrar(EnvPackageElements& elements)
		: m_elements(elements)
	{}

	// IEnvElementRegistrar

	virtual void Register(const IEnvElementPtr& pElement, const SGUID& scopeGUID) override
	{
		SCHEMATYC_CORE_ASSERT(pElement);
		if (pElement)
		{
			m_elements.emplace_back(pElement->GetGUID(), pElement, scopeGUID);
		}
	}

	// ~IEnvElementRegistrar

	// IEnvRegistrar

	virtual CEnvRegistrationScope RootScope() override
	{
		return CEnvRegistrationScope(*this, SGUID());
	}

	virtual CEnvRegistrationScope Scope(const SGUID& scopeGUID) override
	{
		return CEnvRegistrationScope(*this, scopeGUID);
	}

	// ~IEnvRegistrar

private:

	EnvPackageElements& m_elements;
};

CEnvRoot::CEnvRoot()
	: CEnvElementBase(SGUID(), "Root", SCHEMATYC_SOURCE_FILE_INFO)
{}

bool CEnvRoot::IsValidScope(IEnvElement& scope) const
{
	return false;
}

bool CEnvRegistry::RegisterPackage(IEnvPackagePtr&& pPackage)
{
	SCHEMATYC_CORE_ASSERT(pPackage);
	if (!pPackage)
	{
		return false;
	}

	const SGUID guid = pPackage->GetGUID();
	EnvPackageCallback callback = pPackage->GetCallback();

	SCHEMATYC_CORE_ASSERT(!GUID::IsEmpty(guid) && !callback.IsEmpty());
	if (GUID::IsEmpty(guid) || callback.IsEmpty())
	{
		return false;
	}

	const bool bUniqueGUID = !GetPackage(guid);
	SCHEMATYC_CORE_ASSERT(bUniqueGUID);
	if (!bUniqueGUID)
	{
		return false;
	}

	SCHEMATYC_CORE_COMMENT("Registering package: name = %s", pPackage->GetName());

	EnvPackageElements packageElements;
	packageElements.reserve(100);

	CEnvPackageElementRegistrar packageElementRegistrar(packageElements);
	callback(packageElementRegistrar);

	if (RegisterPackageElements(packageElements))
	{
		m_packages.insert(Packages::value_type(guid, std::forward<IEnvPackagePtr>(pPackage)));
		return true;
	}
	else
	{
		ReleasePackageElements(packageElements);
		SCHEMATYC_CORE_CRITICAL_ERROR("Failed to register package!");
		return false;
	}
}

const IEnvPackage* CEnvRegistry::GetPackage(const SGUID& guid) const
{
	Packages::const_iterator itPackage = m_packages.find(guid);
	return itPackage != m_packages.end() ? itPackage->second.get() : nullptr;
}

void CEnvRegistry::VisitPackages(const EnvPackageConstVisitor& visitor) const
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty());
	if (!visitor.IsEmpty())
	{
		for (const Packages::value_type& package : m_packages)
		{
			if (visitor(*package.second) == EVisitStatus::Stop)
			{
				break;
			}
		}
	}
}

const IEnvElement& CEnvRegistry::GetRoot() const
{
	return m_root;
}

const IEnvElement* CEnvRegistry::GetElement(const SGUID& guid) const
{
	Elements::const_iterator itElement = m_elements.find(guid);
	return itElement != m_elements.end() ? itElement->second.get() : nullptr;
}

const IEnvModule* CEnvRegistry::GetModule(const SGUID& guid) const
{
	Modules::const_iterator itModule = m_modules.find(guid);
	return itModule != m_modules.end() ? itModule->second.get() : nullptr;
}

void CEnvRegistry::VisitModules(const EnvModuleConstVisitor& visitor) const
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty());
	if (!visitor.IsEmpty())
	{
		for (const Modules::value_type& module : m_modules)
		{
			if (visitor(*module.second) == EVisitStatus::Stop)
			{
				break;
			}
		}
	}
}

const IEnvDataType* CEnvRegistry::GetDataType(const SGUID& guid) const
{
	DataTypes::const_iterator itDataType = m_dataTypes.find(guid);
	return itDataType != m_dataTypes.end() ? itDataType->second.get() : nullptr;
}

void CEnvRegistry::VisitDataTypes(const EnvDataTypeConstVisitor& visitor) const
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty());
	if (!visitor.IsEmpty())
	{
		for (const DataTypes::value_type& dataType : m_dataTypes)
		{
			if (visitor(*dataType.second) == EVisitStatus::Stop)
			{
				break;
			}
		}
	}
}

const IEnvSignal* CEnvRegistry::GetSignal(const SGUID& guid) const
{
	Signals::const_iterator itSignal = m_signals.find(guid);
	return itSignal != m_signals.end() ? itSignal->second.get() : nullptr;
}

void CEnvRegistry::VisitSignals(const EnvSignalConstVisitor& visitor) const
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty());
	if (!visitor.IsEmpty())
	{
		for (const Signals::value_type& signal : m_signals)
		{
			if (visitor(*signal.second) == EVisitStatus::Stop)
			{
				break;
			}
		}
	}
}

const IEnvFunction* CEnvRegistry::GetFunction(const SGUID& guid) const
{
	Functions::const_iterator itFunction = m_functions.find(guid);
	return itFunction != m_functions.end() ? itFunction->second.get() : nullptr;
}

void CEnvRegistry::VisitFunctions(const EnvFunctionConstVisitor& visitor) const
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty());
	if (!visitor.IsEmpty())
	{
		for (const Functions::value_type& function : m_functions)
		{
			if (visitor(*function.second) == EVisitStatus::Stop)
			{
				break;
			}
		}
	}
}

const IEnvClass* CEnvRegistry::GetClass(const SGUID& guid) const
{
	Classes::const_iterator itClass = m_classes.find(guid);
	return itClass != m_classes.end() ? itClass->second.get() : nullptr;
}

void CEnvRegistry::VisitClasses(const EnvClassConstVisitor& visitor) const
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty());
	if (!visitor.IsEmpty())
	{
		for (const Classes::value_type& _class : m_classes)
		{
			if (visitor(*_class.second) == EVisitStatus::Stop)
			{
				break;
			}
		}
	}
}

const IEnvInterface* CEnvRegistry::GetInterface(const SGUID& guid) const
{
	Interfaces::const_iterator itInterface = m_interfaces.find(guid);
	return itInterface != m_interfaces.end() ? itInterface->second.get() : nullptr;
}

void CEnvRegistry::VisitInterfaces(const EnvInterfaceConstVisitor& visitor) const
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty());
	if (!visitor.IsEmpty())
	{
		for (const Interfaces::value_type& envInterface : m_interfaces)
		{
			if (visitor(*envInterface.second) == EVisitStatus::Stop)
			{
				break;
			}
		}
	}
}

const IEnvInterfaceFunction* CEnvRegistry::GetInterfaceFunction(const SGUID& guid) const
{
	InterfaceFunctions::const_iterator itFunction = m_interfaceFunctions.find(guid);
	return itFunction != m_interfaceFunctions.end() ? itFunction->second.get() : nullptr;
}

void CEnvRegistry::VisitInterfaceFunctions(const EnvInterfaceFunctionConstVisitor& visitor) const
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty());
	if (!visitor.IsEmpty())
	{
		for (const InterfaceFunctions::value_type& function : m_interfaceFunctions)
		{
			if (visitor(*function.second) == EVisitStatus::Stop)
			{
				break;
			}
		}
	}
}

const IEnvComponent* CEnvRegistry::GetComponent(const SGUID& guid) const
{
	Components::const_iterator itComponent = m_components.find(guid);
	return itComponent != m_components.end() ? itComponent->second.get() : nullptr;
}

void CEnvRegistry::VisitComponents(const EnvComponentConstVisitor& visitor) const
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty());
	if (!visitor.IsEmpty())
	{
		for (const Components::value_type& component : m_components)
		{
			if (visitor(*component.second) == EVisitStatus::Stop)
			{
				break;
			}
		}
	}
}

const IEnvAction* CEnvRegistry::GetAction(const SGUID& guid) const
{
	Actions::const_iterator itAction = m_actions.find(guid);
	return itAction != m_actions.end() ? itAction->second.get() : nullptr;
}

void CEnvRegistry::VisitActions(const EnvActionConstVisitor& visitor) const
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty());
	if (!visitor.IsEmpty())
	{
		for (const Actions::value_type& action : m_actions)
		{
			if (visitor(*action.second) == EVisitStatus::Stop)
			{
				break;
			}
		}
	}
}

void CEnvRegistry::BlacklistElement(const SGUID& guid)
{
	m_blacklist.insert(guid);
}

bool CEnvRegistry::IsBlacklistedElement(const SGUID& guid) const
{
	return m_blacklist.find(guid) != m_blacklist.end();
}

void CEnvRegistry::Refresh()
{
	m_actions.clear();
	m_components.clear();
	m_interfaceFunctions.clear();
	m_interfaces.clear();
	m_classes.clear();
	m_functions.clear();
	m_signals.clear();
	m_dataTypes.clear();
	m_modules.clear();

	m_elements.clear();

	for (const Packages::value_type& package : m_packages)
	{
		// #SchematycTODO : Re-register package elements!!!
	}
}

bool CEnvRegistry::RegisterPackageElements(const EnvPackageElements& packageElements)
{
	m_elements.reserve(m_elements.size() + packageElements.size());

	bool bError = false;

	for (const SEnvPackageElement& packageElement : packageElements)
	{
		if (GetElement(packageElement.elementGUID))
		{
			CStackString temp;
			GUID::ToString(temp, packageElement.elementGUID);
			SCHEMATYC_CORE_CRITICAL_ERROR("Duplicate element guid: element = %s, guid = %s", packageElement.pElement->GetName(), temp.c_str());
			bError = true;
		}

		m_elements.insert(Elements::value_type(packageElement.elementGUID, packageElement.pElement));

		switch (packageElement.pElement->GetType())
		{
		case EEnvElementType::Module:
			{
				m_modules.insert(Modules::value_type(packageElement.elementGUID, std::static_pointer_cast<IEnvModule>(packageElement.pElement)));
				break;
			}
		case EEnvElementType::DataType:
			{
				m_dataTypes.insert(DataTypes::value_type(packageElement.elementGUID, std::static_pointer_cast<IEnvDataType>(packageElement.pElement)));
				break;
			}
		case EEnvElementType::Signal:
			{
				m_signals.insert(Signals::value_type(packageElement.elementGUID, std::static_pointer_cast<IEnvSignal>(packageElement.pElement)));
				break;
			}
		case EEnvElementType::Function:
			{
				m_functions.insert(Functions::value_type(packageElement.elementGUID, std::static_pointer_cast<IEnvFunction>(packageElement.pElement)));
				break;
			}
		case EEnvElementType::Class:
			{
				m_classes.insert(Classes::value_type(packageElement.elementGUID, std::static_pointer_cast<IEnvClass>(packageElement.pElement)));
				break;
			}
		case EEnvElementType::Interface:
			{
				m_interfaces.insert(Interfaces::value_type(packageElement.elementGUID, std::static_pointer_cast<IEnvInterface>(packageElement.pElement)));
				break;
			}
		case EEnvElementType::InterfaceFunction:
			{
				m_interfaceFunctions.insert(InterfaceFunctions::value_type(packageElement.elementGUID, std::static_pointer_cast<IEnvInterfaceFunction>(packageElement.pElement)));
				break;
			}
		case EEnvElementType::Component:
			{
				m_components.insert(Components::value_type(packageElement.elementGUID, std::static_pointer_cast<IEnvComponent>(packageElement.pElement)));
				break;
			}
		case EEnvElementType::Action:
			{
				m_actions.insert(Actions::value_type(packageElement.elementGUID, std::static_pointer_cast<IEnvAction>(packageElement.pElement)));
				break;
			}
		}
	}

	for (const SEnvPackageElement& packageElement : packageElements)
	{
		IEnvElement* pScope = nullptr;
		if (!GUID::IsEmpty(packageElement.scopeGUID))
		{
			pScope = GetElement(packageElement.scopeGUID);
		}
		else
		{
			pScope = &m_root;
		}

		if (pScope)
		{
			const char* szElementName = packageElement.pElement->GetName();
			auto matchElementName = [szElementName](const IEnvElement& childElement) -> EVisitStatus
			{
				return strcmp(childElement.GetName(), szElementName) ? EVisitStatus::Continue : EVisitStatus::Stop;
			};
			if (pScope->VisitChildren(EnvElementConstVisitor::FromLambda(matchElementName)) == EVisitStatus::Stop)
			{
				SCHEMATYC_CORE_WARNING("Duplicate element name: element = %s, scope = %s", szElementName, pScope->GetName());
			}
		}

		if (!pScope || !pScope->AttachChild(*packageElement.pElement))
		{
			CStackString temp;
			GUID::ToString(temp, packageElement.elementGUID);
			SCHEMATYC_CORE_CRITICAL_ERROR("Invalid element scope: element = %s, scope = %s", packageElement.pElement->GetName(), temp.c_str());
			bError = true;
		}
	}

	if(!ValidateComponentDependencies())
	{
		bError = true;
	}

	return !bError;
}

void CEnvRegistry::ReleasePackageElements(const EnvPackageElements& packageElements)
{
	for (const SEnvPackageElement& packageElement : packageElements)
	{
		IEnvElement* pScope = packageElement.pElement->GetParent();
		if (pScope)
		{
			pScope->DetachChild(*packageElement.pElement);
		}

		switch (packageElement.pElement->GetType())
		{
		case EEnvElementType::Module:
			{
				m_modules.erase(packageElement.elementGUID);
				break;
			}
		case EEnvElementType::DataType:
			{
				m_dataTypes.erase(packageElement.elementGUID);
				break;
			}
		case EEnvElementType::Signal:
			{
				m_signals.erase(packageElement.elementGUID);
				break;
			}
		case EEnvElementType::Function:
			{
				m_functions.erase(packageElement.elementGUID);
				break;
			}
		case EEnvElementType::Class:
			{
				m_classes.erase(packageElement.elementGUID);
				break;
			}
		case EEnvElementType::Interface:
			{
				m_interfaces.erase(packageElement.elementGUID);
				break;
			}
		case EEnvElementType::InterfaceFunction:
			{
				m_interfaceFunctions.erase(packageElement.elementGUID);
				break;
			}
		case EEnvElementType::Component:
			{
				m_components.erase(packageElement.elementGUID);
				break;
			}
		case EEnvElementType::Action:
			{
				m_actions.erase(packageElement.elementGUID);
				break;
			}
		}

		m_elements.erase(packageElement.elementGUID);
	}
}

bool CEnvRegistry::ValidateComponentDependencies() const
{
	bool bError = false;

	for (const Components::value_type& component : m_components)
	{
		const CComponentDesc& componentDesc = component.second->GetDesc();
		for (uint32 dependencyIdx = 0, dependencyCount = componentDesc.GetDependencyCount(); dependencyIdx < dependencyCount; ++dependencyIdx)
		{
			const SGUID dependencyGUID = componentDesc.GetDependency(dependencyIdx)->guid;
			const IEnvComponent* pDependencyComponent = GetComponent(dependencyGUID);
			if (pDependencyComponent)
			{
				const CComponentDesc& dependencyComponentDesc = pDependencyComponent->GetDesc();
				if (!dependencyComponentDesc.GetComponentFlags().Check(EComponentFlags::Singleton))
				{
					SCHEMATYC_CORE_CRITICAL_ERROR("Non-singleton component detected as dependency: component = %s, dependency = %s", component.second->GetName(), pDependencyComponent->GetName());
					bError = true;
				}
				if (dependencyComponentDesc.IsDependency(component.first))
				{
					SCHEMATYC_CORE_CRITICAL_ERROR("Circular dependency detected between components: component = %s, dependency = %s", component.second->GetName(), pDependencyComponent->GetName());
					bError = true;
				}
			}
			else
			{
				CStackString temp;
				GUID::ToString(temp, dependencyGUID);
				SCHEMATYC_CORE_CRITICAL_ERROR("Unable to resolve component dependency: component = %s, dependency = %s", component.second->GetName(), temp.c_str());
				bError = true;
			}
		}
	}

	return !bError;
}

IEnvElement* CEnvRegistry::GetElement(const SGUID& guid)
{
	Elements::iterator itElement = m_elements.find(guid);
	return itElement != m_elements.end() ? itElement->second.get() : nullptr;
}
} // Schematyc
