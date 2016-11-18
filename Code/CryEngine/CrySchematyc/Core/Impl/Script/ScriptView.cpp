// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #Schematyc : Ditch visit status support or implement correct visitor status handling!!!

#include "StdAfx.h"
#include "ScriptView.h"

#include <Schematyc/FundamentalTypes.h>
#include <Schematyc/ICore.h>
#include <Schematyc/Env/IEnvRegistry.h>
#include <Schematyc/Env/Elements/IEnvAction.h>
#include <Schematyc/Env/Elements/IEnvClass.h>
#include <Schematyc/Env/Elements/IEnvComponent.h>
#include <Schematyc/Env/Elements/IEnvDataType.h>
#include <Schematyc/Env/Elements/IEnvFunction.h>
#include <Schematyc/Script/IScriptGraph.h>
#include <Schematyc/Script/IScriptRegistry.h>
#include <Schematyc/Script/Elements/IScriptActionInstance.h>
#include <Schematyc/Script/Elements/IScriptBase.h>
#include <Schematyc/Script/Elements/IScriptClass.h>
#include <Schematyc/Script/Elements/IScriptComponentInstance.h>
#include <Schematyc/Script/Elements/IScriptEnum.h>
#include <Schematyc/Script/Elements/IScriptFunction.h>
#include <Schematyc/Script/Elements/IScriptModule.h>
#include <Schematyc/Script/Elements/IScriptSignal.h>
#include <Schematyc/Script/Elements/IScriptState.h>
#include <Schematyc/Script/Elements/IScriptStateMachine.h>
#include <Schematyc/Script/Elements/IScriptStruct.h>
#include <Schematyc/Script/Elements/IScriptTimer.h>
#include <Schematyc/Script/Elements/IScriptVariable.h>
#include <Schematyc/Utils/Assert.h>
#include <Schematyc/Utils/EnumFlags.h>

#include "CVars.h"
#include "Services/Log.h"

namespace Schematyc
{
namespace ScriptViewUtils_DEPRECATED
{
enum class EVisitFlags
{
	VisitParents  = BIT(0),
	VisitChildren = BIT(1)
};

typedef CEnumFlags<EVisitFlags> VisitFlags;

inline const IScriptClass* GetScriptBaseClass(const IScriptElement& scriptElement)
{
	SElementId baseClassId;
	auto visitScriptElement = [&baseClassId](const IScriptElement& scriptElement) -> EVisitStatus
	{
		if (scriptElement.GetElementType() == EScriptElementType::Base)
		{
			baseClassId = DynamicCast<IScriptBase>(scriptElement).GetClassId();
			return EVisitStatus::Stop;
		}
		return EVisitStatus::Continue;
	};
	scriptElement.VisitChildren(ScriptElementConstVisitor::FromLambda(visitScriptElement));
	return baseClassId.domain == EDomain::Script ? DynamicCast<IScriptClass>(GetSchematycCore().GetScriptRegistry().GetElement(baseClassId.guid)) : nullptr;
}

// #SchematycTODO : No need to pass element type, this can be retrieved using ELEMENT::ElementType.
template<typename ELEMENT> void VisitScriptElements(const CDelegate<EVisitStatus(const ELEMENT&)>& visitor, EScriptElementType filterElementType, EScriptElementAccessor accessor, const IScriptElement& scriptScope)
{
	for (const IScriptElement* pScriptElement = scriptScope.GetFirstChild(); pScriptElement; pScriptElement = pScriptElement->GetNextSibling())
	{
		if (pScriptElement->GetAccessor() <= accessor)
		{
			VisitScriptElements<ELEMENT>(visitor, filterElementType, accessor, *pScriptElement);
			if (pScriptElement->GetElementType() == filterElementType)
			{
				if (visitor(static_cast<const ELEMENT&>(*pScriptElement)) == EVisitStatus::Stop)
				{
					break;
				}
			}
		}
	}
}

// #SchematycTODO : No need to pass element type, this can be retrieved using ELEMENT::ElementType.
template<typename ELEMENT> void VisitScriptElementsInClass(const CDelegate<EVisitStatus(const ELEMENT&)>& visitor, EScriptElementType elementType, EDomainScope scope, SGUID scopeGUID)
{
	const IScriptElement* pScriptElement = GetSchematycCore().GetScriptRegistry().GetElement(scopeGUID);
	while (pScriptElement)
	{
		VisitScriptElements<ELEMENT>(visitor, elementType, EScriptElementAccessor::Private, *pScriptElement);
		if (pScriptElement->GetElementType() != EScriptElementType::Class)
		{
			pScriptElement = pScriptElement->GetParent();
		}
		else
		{
			break;
		}
	}
	if (pScriptElement && ((scope == EDomainScope::Derived) || ((scope == EDomainScope::All))))
	{
		const EScriptElementAccessor accessor = scope == EDomainScope::Derived ? EScriptElementAccessor::Protected : EScriptElementAccessor::Private;
		for (const IScriptClass* pScriptBaseClass = GetScriptBaseClass(*pScriptElement); pScriptBaseClass; pScriptBaseClass = GetScriptBaseClass(*pScriptBaseClass))
		{
			VisitScriptElements<ELEMENT>(visitor, elementType, accessor, *pScriptBaseClass);
		}
	}
}

template<typename ELEMENT> void VisitScriptElementsRecursive(const CDelegate<EVisitStatus(const ELEMENT&)>& visitor, const IScriptElement& scope, EScriptElementType type, EScriptElementAccessor accessor, const VisitFlags& flags, const IScriptElement* pSkipElement)
{
	for (const IScriptElement* pScriptElement = scope.GetFirstChild(); pScriptElement; pScriptElement = pScriptElement->GetNextSibling())
	{
		if (pScriptElement != pSkipElement)
		{
			const EScriptElementType elementType = pScriptElement->GetElementType();

			if ((elementType == type) && (pScriptElement->GetAccessor() <= accessor))
			{
				if (visitor(static_cast<const ELEMENT&>(*pScriptElement)) == EVisitStatus::Stop)
				{
					break;
				}
			}

			if (flags.Check(EVisitFlags::VisitChildren))
			{
				VisitScriptElementsRecursive<ELEMENT>(visitor, *pScriptElement, type, accessor, EVisitFlags::VisitChildren, nullptr);
			}

			switch (elementType)
			{
			case EScriptElementType::Base:
				{
					const SElementId baseClassId = DynamicCast<IScriptBase>(pScriptElement)->GetClassId();
					if (baseClassId.domain == EDomain::Script)
					{
						const IScriptElement* pScriptClass = GetSchematycCore().GetScriptRegistry().GetElement(baseClassId.guid);
						if (pScriptClass)
						{
							VisitScriptElementsRecursive<ELEMENT>(visitor, *pScriptClass, type, EScriptElementAccessor::Protected, EVisitFlags::VisitChildren, nullptr);
						}
					}
					break;
				}
			}
		}
	}

	if (flags.Check(EVisitFlags::VisitParents))
	{
		const IScriptElement* pParentScriptElement = scope.GetParent();
		if (pParentScriptElement)
		{
			VisitScriptElementsRecursive<ELEMENT>(visitor, *pParentScriptElement, type, accessor, EVisitFlags::VisitParents, &scope);
		}
	}
}

template<typename ELEMENT> void VisitScriptElements(const CDelegate<EVisitStatus(const ELEMENT&)>& visitor, EScriptElementType type, const SGUID& scopeGUID)
{
	const IScriptElement* pScriptScope = GetSchematycCore().GetScriptRegistry().GetElement(scopeGUID);
	if (pScriptScope)
	{
		VisitScriptElementsRecursive<ELEMENT>(visitor, *pScriptScope, type, EScriptElementAccessor::Private, { EVisitFlags::VisitParents, EVisitFlags::VisitChildren }, nullptr);
	}
}
} // ScriptViewUtils_DEPRECATED

namespace ScriptViewUtils
{
enum class EVisitFlags
{
	VisitParents  = BIT(0),
	VisitChildren = BIT(1)
};

typedef CEnumFlags<EVisitFlags>            VisitFlags;
typedef std::vector<const IScriptElement*> ScriptAncestors;

template<typename ELEMENT> void VisitElementsRecursive(const IScriptElement& scope, const CDelegate<void(const ELEMENT&)>& visitor, EScriptElementAccessor accessor, const VisitFlags& flags, const IScriptElement* pSkipElement)
{
	for (const IScriptElement* pScriptElement = scope.GetFirstChild(); pScriptElement; pScriptElement = pScriptElement->GetNextSibling())
	{
		if (pScriptElement != pSkipElement)
		{
			if (accessor >= pScriptElement->GetAccessor())
			{
				const ELEMENT* pScriptElementImpl = DynamicCast<ELEMENT>(pScriptElement);
				if (pScriptElementImpl)
				{
					visitor(*pScriptElementImpl);
				}

				if (flags.Check(EVisitFlags::VisitChildren))
				{
					VisitElementsRecursive<ELEMENT>(*pScriptElement, visitor, EScriptElementAccessor::Public, EVisitFlags::VisitChildren, nullptr);
				}

				if (accessor >= EScriptElementAccessor::Protected)
				{
					switch (pScriptElement->GetElementType())
					{
					case EScriptElementType::Base:
						{
							const SElementId baseClassId = DynamicCast<IScriptBase>(pScriptElement)->GetClassId();
							if (baseClassId.domain == EDomain::Script)
							{
								const IScriptElement* pScriptClass = GetSchematycCore().GetScriptRegistry().GetElement(baseClassId.guid);
								if (pScriptClass)
								{
									VisitElementsRecursive<ELEMENT>(*pScriptClass, visitor, EScriptElementAccessor::Protected, EVisitFlags::VisitChildren, nullptr);
								}
							}
							break;
						}
					}
				}
			}
		}
	}

	if (flags.Check(EVisitFlags::VisitParents))
	{
		const IScriptElement* pParentScriptElement = scope.GetParent();
		if (pParentScriptElement)
		{
			VisitElementsRecursive<ELEMENT>(*pParentScriptElement, visitor, accessor, flags, &scope);
		}
	}
}

template<typename ELEMENT> void VisitEnclosedElememts(const SGUID& scopeGUID, const CDelegate<void(const ELEMENT&)>& visitor)
{
	const IScriptElement* pScriptScope = GetSchematycCore().GetScriptRegistry().GetElement(scopeGUID);
	if (pScriptScope)
	{
		VisitElementsRecursive<ELEMENT>(*pScriptScope, visitor, EScriptElementAccessor::Private, EVisitFlags::VisitChildren, nullptr);
	}
}

template<typename ELEMENT> void VisitAccesibleElememts(const SGUID& scopeGUID, const CDelegate<void(const ELEMENT&)>& visitor)
{
	const IScriptElement* pScriptScope = GetSchematycCore().GetScriptRegistry().GetElement(scopeGUID);
	if (pScriptScope)
	{
		VisitElementsRecursive<ELEMENT>(*pScriptScope, visitor, EScriptElementAccessor::Private, { EVisitFlags::VisitParents, EVisitFlags::VisitChildren }, nullptr);
	}
}

inline void GetScriptAncestors(const IScriptElement& scriptElement, ScriptAncestors& ancestors)
{
	ancestors.reserve(16);
	ancestors.push_back(&scriptElement);
	for (const IScriptElement* pScriptScope = scriptElement.GetParent(); pScriptScope; pScriptScope = pScriptScope->GetParent())
	{
		ancestors.push_back(pScriptScope);
	}
}

inline const IScriptElement* FindFirstCommonScriptAncestor(const IScriptElement& lhsScriptElement, const IScriptElement& rhsScriptElement)
{
	ScriptAncestors lhsScriptAncestors;
	ScriptAncestors rhsScriptAncestors;
	GetScriptAncestors(lhsScriptElement, lhsScriptAncestors);
	GetScriptAncestors(rhsScriptElement, rhsScriptAncestors);
	for (const IScriptElement* pScriptAncestor : lhsScriptAncestors)
	{
		if (std::find(rhsScriptAncestors.begin(), rhsScriptAncestors.end(), pScriptAncestor) != rhsScriptAncestors.end())
		{
			return pScriptAncestor;
		}
	}
	return nullptr;
}

inline bool QualifyScriptElementName(const IScriptElement& scriptScope, const IScriptElement& scriptElement, EDomainQualifier qualifier, IString& output)
{
	output.clear();
	switch (qualifier)
	{
	case EDomainQualifier::Global:
		{
			output.assign(scriptElement.GetName());

			for (const IScriptElement* pScriptScope = scriptElement.GetParent(); pScriptScope && (pScriptScope->GetElementType() != EScriptElementType::Root); pScriptScope = pScriptScope->GetParent())
			{
				output.insert(0, "::");
				output.insert(0, pScriptScope->GetName());
			}
			return true;
		}
	case EDomainQualifier::Local:
		{
			if (&scriptScope != &scriptElement)
			{
				output.assign(scriptElement.GetName());

				const IScriptElement* pFirstCommonScriptAncestor = FindFirstCommonScriptAncestor(scriptScope, scriptElement);
				for (const IScriptElement* pScriptScope = scriptElement.GetParent(); (pScriptScope != pFirstCommonScriptAncestor) && (pScriptScope->GetElementType() != EScriptElementType::Root); pScriptScope = pScriptScope->GetParent())
				{
					output.insert(0, "::");
					output.insert(0, pScriptScope->GetName());
				}
			}
			return true;
		}
	default:
		{
			return false;
		}
	}
}
} //ScriptViewUtils

CScriptView::CScriptView(const SGUID& scopeGUID)
	: m_scopeGUID(scopeGUID)
{}

const SGUID& CScriptView::GetScopeGUID() const
{
	return m_scopeGUID;
}

const IEnvClass* CScriptView::GetEnvClass() const
{
	IScriptRegistry& scriptRegistry = GetSchematycCore().GetScriptRegistry();
	const IScriptClass* pScriptClass = GetScriptClass();
	SElementId baseClassId;
	while (pScriptClass)
	{
		auto visitScriptElement = [&baseClassId](const IScriptElement& scriptElement) -> EVisitStatus
		{
			if (scriptElement.GetElementType() == EScriptElementType::Base)
			{
				baseClassId = DynamicCast<IScriptBase>(scriptElement).GetClassId();
				return EVisitStatus::Stop;
			}
			return EVisitStatus::Continue;
		};
		pScriptClass->VisitChildren(ScriptElementConstVisitor::FromLambda(visitScriptElement));
		if (baseClassId.domain == EDomain::Script)
		{
			pScriptClass = DynamicCast<IScriptClass>(scriptRegistry.GetElement(baseClassId.guid));
		}
		else
		{
			break;
		}
	}
	return baseClassId.domain == EDomain::Env ? GetSchematycCore().GetEnvRegistry().GetClass(baseClassId.guid) : nullptr;
}

const IScriptClass* CScriptView::GetScriptClass() const
{
	const IScriptElement* pScriptElement = GetSchematycCore().GetScriptRegistry().GetElement(m_scopeGUID);
	while (pScriptElement)
	{
		if (pScriptElement->GetElementType() == EScriptElementType::Class)
		{
			return DynamicCast<IScriptClass>(pScriptElement);
		}
		else
		{
			pScriptElement = pScriptElement->GetParent();
		}
	}
	return nullptr;
}

void CScriptView::VisitEnvDataTypes(const EnvDataTypeConstVisitor& visitor) const
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty())
	if (!visitor.IsEmpty())
	{
		//const IEnvClass* pEnvClass = GetEnvClass();
		auto visitEnvDataType = [&visitor /*, &pEnvClass*/](const IEnvDataType& envDataType) -> EVisitStatus
		{
			//if(!pEnvClass || ScriptViewUtils::IsEnvClassUsingNamespace(*pEnvClass, envDataType.GetNamespace()))
			{
				return visitor(envDataType);
			}
			return EVisitStatus::Continue;
		};
		GetSchematycCore().GetEnvRegistry().VisitDataTypes(EnvDataTypeConstVisitor::FromLambda(visitEnvDataType));
	}
}

void CScriptView::VisitEnvSignals(const EnvSignalConstVisitor& visitor) const
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty())
	if (!visitor.IsEmpty())
	{
		//const IEnvSignal* pEnvClass = GetEnvClass();
		auto visitEnvSignal = [&visitor /*, &pEnvClass*/](const IEnvSignal& envSignal) -> EVisitStatus
		{
			//if(!pEnvClass || ScriptViewUtils::IsEnvClassUsingNamespace(*pEnvClass, envSignal.GetNamespace()))
			{
				return visitor(envSignal);
			}
			return EVisitStatus::Continue;
		};
		GetSchematycCore().GetEnvRegistry().VisitSignals(EnvSignalConstVisitor::FromLambda(visitEnvSignal));
	}
}

void CScriptView::VisitEnvFunctions(const EnvFunctionConstVisitor& visitor) const
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty())
	if (!visitor.IsEmpty())
	{
		auto visitEnvFunction = [&visitor](const IEnvFunction& envFunction) -> EVisitStatus
		{
			// #SchematycTODO : Filter based on scope!
			return visitor(envFunction);
		};
		GetSchematycCore().GetEnvRegistry().VisitFunctions(EnvFunctionConstVisitor::FromLambda(visitEnvFunction));
	}
}

void CScriptView::VisitEnvClasses(const EnvClassConstVisitor& visitor) const
{
	GetSchematycCore().GetEnvRegistry().VisitClasses(visitor);
}

void CScriptView::VisitEnvInterfaces(const EnvInterfaceConstVisitor& visitor) const
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty())
	if (!visitor.IsEmpty())
	{
		const IEnvClass* pEnvClass = GetEnvClass();
		auto visitEnvInterface = [&visitor, &pEnvClass](const IEnvInterface& envInterface) -> EVisitStatus
		{
			if (!pEnvClass /* || ScriptViewUtils::IsEnvClassUsingNamespace(*pEnvClass, envInterface.GetNamespace())*/)
			{
				return visitor(envInterface);
			}
			return EVisitStatus::Continue;
		};
		GetSchematycCore().GetEnvRegistry().VisitInterfaces(EnvInterfaceConstVisitor::FromLambda(visitEnvInterface));
	}
}

void CScriptView::VisitEnvComponents(const EnvComponentConstVisitor& visitor) const
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty())
	if (!visitor.IsEmpty())
	{
		typedef std::vector<const IEnvComponent*> Exclusions;

		Exclusions exclusions;
		auto visitScriptComponentInstances = [&visitor, &exclusions](const IScriptComponentInstance& componentInstance) -> EVisitStatus
		{
			const IEnvComponent* pEnvComponent = GetSchematycCore().GetEnvRegistry().GetComponent(componentInstance.GetTypeGUID());
			if (pEnvComponent && pEnvComponent->GetFlags().Check(EEnvComponentFlags::Singleton))
			{
				exclusions.push_back(pEnvComponent);
			}
			return EVisitStatus::Continue;
		};
		VisitScriptComponentInstances(ScriptComponentInstanceConstVisitor::FromLambda(visitScriptComponentInstances), EDomainScope::All);

		const IEnvClass* pEnvClass = GetEnvClass();
		auto visitEnvComponent = [&visitor, &exclusions, &pEnvClass](const IEnvComponent& envComponent) -> EVisitStatus
		{
			if (std::find(exclusions.begin(), exclusions.end(), &envComponent) == exclusions.end())
			{
				//if (!pEnvClass || ScriptViewUtils::IsEnvClassUsingNamespace(*pEnvClass, envComponent.GetNamespace()))
				//{
				return visitor(envComponent);
				//}
			}
			return EVisitStatus::Continue;
		};
		GetSchematycCore().GetEnvRegistry().VisitComponents(EnvComponentConstVisitor::FromLambda(visitEnvComponent));
	}
}

void CScriptView::VisitEnvActions(const EnvActionConstVisitor& visitor) const
{
	// #SchematycTODO : Filter actions based on env modules and script component instances?

	GetSchematycCore().GetEnvRegistry().VisitActions(visitor);
}

void CScriptView::VisitScriptModules(const ScriptModuleConstVisitor& visitor) const
{
	ScriptViewUtils_DEPRECATED::VisitScriptElements<IScriptModule>(visitor, EScriptElementType::Module, m_scopeGUID);
}

void CScriptView::VisitScriptFunctions(const ScriptFunctionConstVisitor& visitor) const
{
	ScriptViewUtils_DEPRECATED::VisitScriptElements<IScriptFunction>(visitor, EScriptElementType::Function, m_scopeGUID);
}

void CScriptView::VisitScriptClasses(const ScriptClassConstVisitor& visitor, EDomainScope scope) const
{
	ScriptViewUtils_DEPRECATED::VisitScriptElementsInClass<IScriptClass>(visitor, EScriptElementType::Class, scope, m_scopeGUID);   // #SchematycTODO : Replace with VisitScriptElements?
}

void CScriptView::VisitScriptStates(const ScriptStateConstVisitor& visitor, EDomainScope scope) const
{
	ScriptViewUtils_DEPRECATED::VisitScriptElementsInClass<IScriptState>(visitor, EScriptElementType::State, scope, m_scopeGUID);   // #SchematycTODO : Replace with VisitScriptElements?
}

//void CScriptView::VisitScriptStateMachines(const ScriptStateMachineConstVisitor& visitor, EDomainScope scope) const {}

void CScriptView::VisitScriptVariables(const ScriptVariableConstVisitor& visitor, EDomainScope scope) const
{
	ScriptViewUtils_DEPRECATED::VisitScriptElementsInClass<IScriptVariable>(visitor, EScriptElementType::Variable, scope, m_scopeGUID);   // #SchematycTODO : Replace with VisitScriptElements?
}

void CScriptView::VisitScriptComponentInstances(const ScriptComponentInstanceConstVisitor& visitor, EDomainScope scope) const
{
	ScriptViewUtils_DEPRECATED::VisitScriptElementsInClass<IScriptComponentInstance>(visitor, EScriptElementType::ComponentInstance, scope, m_scopeGUID);
}

void CScriptView::VisitScriptActionInstances(const ScriptActionInstanceConstVisitor& visitor, EDomainScope scope) const
{
	ScriptViewUtils_DEPRECATED::VisitScriptElementsInClass<IScriptActionInstance>(visitor, EScriptElementType::ActionInstance, scope, m_scopeGUID);
}

void CScriptView::VisitEnclosedEnums(const ScriptEnumConstVisitor& visitor) const
{
	ScriptViewUtils::VisitEnclosedElememts<IScriptEnum>(m_scopeGUID, visitor);
}

void CScriptView::VisitAccesibleEnums(const ScriptEnumConstVisitor& visitor) const
{
	ScriptViewUtils::VisitAccesibleElememts<IScriptEnum>(m_scopeGUID, visitor);
}

void CScriptView::VisitEnclosedStructs(const ScriptStructConstVisitor& visitor) const
{
	ScriptViewUtils::VisitEnclosedElememts<IScriptStruct>(m_scopeGUID, visitor);
}

void CScriptView::VisitAccesibleStructs(const ScriptStructConstVisitor& visitor) const
{
	ScriptViewUtils::VisitAccesibleElememts<IScriptStruct>(m_scopeGUID, visitor);
}

void CScriptView::VisitEnclosedSignals(const ScriptSignalConstVisitor& visitor) const
{
	ScriptViewUtils::VisitEnclosedElememts<IScriptSignal>(m_scopeGUID, visitor);
}

void CScriptView::VisitAccesibleSignals(const ScriptSignalConstVisitor& visitor) const
{
	ScriptViewUtils::VisitAccesibleElememts<IScriptSignal>(m_scopeGUID, visitor);
}

void CScriptView::VisitEnclosedTimers(const ScriptTimerConstVisitor& visitor) const
{
	ScriptViewUtils::VisitEnclosedElememts<IScriptTimer>(m_scopeGUID, visitor);
}

void CScriptView::VisitAccesibleTimers(const ScriptTimerConstVisitor& visitor) const
{
	ScriptViewUtils::VisitAccesibleElememts<IScriptTimer>(m_scopeGUID, visitor);
}

const IScriptStateMachine* CScriptView::GetScriptStateMachine(const SGUID& guid) const
{
	return nullptr;
}

const IScriptComponentInstance* CScriptView::GetScriptComponentInstance(const SGUID& guid) const
{
	return DynamicCast<IScriptComponentInstance>(GetSchematycCore().GetScriptRegistry().GetElement(guid));   // #SchematycTODO : Should we be checking to make sure the result is in scope?
}

const IScriptActionInstance* CScriptView::GetScriptActionInstance(const SGUID& guid) const
{
	return nullptr;
}

const IScriptElement* CScriptView::GetScriptElement(const SGUID& guid) const
{
	return GetSchematycCore().GetScriptRegistry().GetElement(guid);   // #SchematycTODO : Should we be checking to make sure the result is in scope?
}

bool CScriptView::QualifyName(const IScriptComponentInstance& scriptComponentInstance, const IEnvFunction& envFunction, EDomainQualifier qualifier, IString& output) const
{
	const IEnvComponent* pEnvComponent = GetSchematycCore().GetEnvRegistry().GetComponent(scriptComponentInstance.GetTypeGUID());
	if (pEnvComponent)
	{
		if (QualifyName(scriptComponentInstance, qualifier, output))
		{
			if (!output.empty())
			{
				output.append("::");
			}
			output.append(envFunction.GetName());
			return true;
		}
	}
	return false;
}

bool CScriptView::QualifyName(const IEnvClass& envClass, IString& output) const
{
	output.clear();
	//output.append(envClass.GetNamespace());
	//output.append("::");
	output.append(envClass.GetName());
	return true;
}

bool CScriptView::QualifyName(const IEnvInterface& envInterface, IString& output) const
{
	output.clear();
	//output.append(envInterface.GetNamespace());
	//output.append("::");
	output.append(envInterface.GetName());
	return true;
}

bool CScriptView::QualifyName(const IEnvInterfaceFunction& envInterfaceFunction, IString& output) const
{
	// #SchematycTODO : Create a recursive function for qualifying all environment element type names.
	const IEnvInterface* pEnvInterface = DynamicCast<IEnvInterface>(envInterfaceFunction.GetParent());
	if (pEnvInterface)
	{
		if (QualifyName(*pEnvInterface, output))
		{
			output.append("::");
			output.append(envInterfaceFunction.GetName());
			return true;
		}
	}
	return false;
}

bool CScriptView::QualifyName(const IEnvComponent& envComponent, IString& output) const
{
	output.clear();
	//output.append(envComponent.GetNamespace());
	//output.append("::");
	output.append(envComponent.GetName());
	return true;
}

void CScriptView::QualifyName(const IEnvElement& envElement, IString& output) const
{
	output.assign(envElement.GetName());
	for (const IEnvElement* pEnvScope = envElement.GetParent(); pEnvScope && (pEnvScope->GetElementType() != EEnvElementType::Root); pEnvScope = pEnvScope->GetParent())
	{
		output.insert(0, "::");
		output.insert(0, pEnvScope->GetName());
	}
}

bool CScriptView::QualifyName(const IScriptElement& scriptElement, EDomainQualifier qualifier, IString& output) const
{
	const IScriptElement* pScriptScope = GetSchematycCore().GetScriptRegistry().GetElement(m_scopeGUID);
	if (pScriptScope)
	{
		return ScriptViewUtils::QualifyScriptElementName(*pScriptScope, scriptElement, qualifier, output);
	}
	return false;
}
} // Schematyc
