#include "stdafx.h"
#include "FlowGraphComponent.h"
#include "Entity.h"
#include <CrySchematyc/CoreAPI.h>

namespace FlowGraphComponentUtils
{
	enum class EInputPorts
	{
		Call,
		ComponentIndex,
		DefaultPortCount
	};

	enum class ESignalOutputPorts
	{
		Received,
		SignalData,
	};

	CryGUID GetSignalEntityClassGUID()
	{
		static CryGUID guid;

		if (guid.IsNull() && gEnv->pSchematyc)
		{
			Schematyc::IScriptRegistry& scriptRegistry = gEnv->pSchematyc->GetScriptRegistry();

			Schematyc::SElementId baseClassId;
			auto visitEnvClass = [&baseClassId](const Schematyc::IEnvClass& envClass) -> Schematyc::EVisitStatus
			{
				baseClassId = Schematyc::SElementId(Schematyc::EDomain::Env, envClass.GetGUID());
				return Schematyc::EVisitStatus::Stop;
			};
			Schematyc::IScriptViewPtr pScriptView = gEnv->pSchematyc->CreateScriptView(scriptRegistry.GetRootElement().GetGUID());
			pScriptView->VisitEnvClasses(visitEnvClass);

			Schematyc::IScriptClass* pClass = scriptRegistry.AddClass("Test", baseClassId, "");
			if (pClass)
			{
				guid = pClass->GetGUID();
				return guid;
			}
		}

		return guid;
	}

	template<typename T>
	void GetValueFromComponent(IEntityComponent* pComponent, size_t index, T& value, const Schematyc::CClassMemberDesc& classDesc, ptrdiff_t offset)
	{
		// TODO: use serializing function otherwise this will break C# components
		classDesc.GetTypeDesc().GetOperators().copyAssign(&value, reinterpret_cast<uint8*>(pComponent) + offset);
	}

	template<typename T>
	void SetComponentValue(IEntityComponent* pComponent, size_t index, T& value, const Schematyc::CClassMemberDesc& classDesc, ptrdiff_t offset)
	{
		// TODO: use serializing function otherwise this will break C# components
		classDesc.GetTypeDesc().GetOperators().copyAssign(reinterpret_cast<uint8*>(pComponent) + offset, &value);
	}

	template<typename T>
	void GetValueFromSignal(const Schematyc::SObjectSignal* pSignal, size_t index, T& tValue)
	{
		int indexIt = 0;

		auto visitInput = [&](const Schematyc::CUniqueId& id, const Schematyc::CAnyConstRef& value)
		{
			if (index == indexIt)
			{
				value.GetTypeDesc().GetOperators().copyAssign(&tValue, value.GetValue());
			}

			++indexIt;
		};

		pSignal->params.VisitInputs(visitInput);
	}

	template<typename T>
	void ActivateSignalOutput(const Schematyc::SObjectSignal* pSignal, IFlowNode::SActivationInfo* pActivationInfo, size_t index)
	{
		T value;
		FlowGraphComponentUtils::GetValueFromSignal(pSignal, index, value);
		ActivateOutput(pActivationInfo, static_cast<int>(ESignalOutputPorts::SignalData) + index, TFlowInputData(value));
	}

	template<typename T>
	void ActivateGetterOutput(IEntityComponent* pComponent, const Schematyc::CClassMemberDesc& classDesc, IFlowNode::SActivationInfo* pActivationInfo, size_t index, ptrdiff_t offset)
	{
		T value;
		FlowGraphComponentUtils::GetValueFromComponent(pComponent, index, value, classDesc, offset);
		ActivateOutput(pActivationInfo, index, TFlowInputData(value));
	}

	// Function with breaks up the Schematyc types into Flowgraph types and executes the lambda function with the flowgraph type,
	// the member description and the actual offset of the variable in memory.
	template<typename LambdaFunction>
	void ExecuteOnType(const Schematyc::CClassMemberDesc& classMemberDesc, ptrdiff_t variableOffset, LambdaFunction functionToExecute)
	{
		auto& typeDesc = classMemberDesc.GetTypeDesc();

		if (typeDesc.GetGUID() == Schematyc::GetTypeGUID<int>())
		{
			functionToExecute(eFDT_Int, classMemberDesc, variableOffset);
		}
		else if (typeDesc.GetGUID() == Schematyc::GetTypeGUID<float>())
		{
			functionToExecute(eFDT_Float, classMemberDesc, variableOffset);
		}
		else if (typeDesc.GetGUID() == Schematyc::GetTypeGUID<Schematyc::ExplicitEntityId>())
		{
			functionToExecute(eFDT_EntityId, classMemberDesc, variableOffset);
		}
		else if (typeDesc.GetGUID() == Schematyc::GetTypeGUID<Vec3>())
		{
			functionToExecute(eFDT_Vec3, classMemberDesc, variableOffset);
		}
		else if (typeDesc.GetGUID() == Schematyc::GetTypeGUID<Schematyc::CSharedString>())
		{
			functionToExecute(eFDT_String, classMemberDesc, variableOffset);
		}
		else if (typeDesc.GetGUID() == Schematyc::GetTypeGUID<bool>())
		{
			functionToExecute(eFDT_Bool, classMemberDesc, variableOffset);
		}
		else if (typeDesc.GetGUID() == Schematyc::GetTypeGUID<CryTransform::CAngle>())
		{
			functionToExecute(eFDT_Float, classMemberDesc, variableOffset);
		}
		else if (typeDesc.GetGUID() == Schematyc::GetTypeGUID<Schematyc::Range<0, 0>>())
		{
			functionToExecute(eFDT_Float, classMemberDesc, variableOffset);
		}
		else if (typeDesc.GetGUID() == Schematyc::GetTypeGUID<Schematyc::Range<0, 0, 0, 0, int>>())
		{
			functionToExecute(eFDT_Int, classMemberDesc, variableOffset);
		}
		else if (typeDesc.GetCategory() == Schematyc::ETypeCategory::Class)
		{
			const Schematyc::IEnvElement* pElement = gEnv->pSchematyc->GetEnvRegistry().GetElement(typeDesc.GetGUID());

			if (pElement != nullptr)
			{
				const stack_string name = pElement->GetName();
				if (name == "Color" || name == "Vector3")
				{
					functionToExecute(eFDT_Vec3, classMemberDesc, variableOffset);
				}
				else if (name.find("Name") != string::npos)
				{
					functionToExecute(eFDT_String, classMemberDesc, variableOffset);
				}
			}
			else
			{
				const Schematyc::CClassMemberDescArray& array = static_cast<const Schematyc::CClassDesc&>(typeDesc).GetMembers();

				ptrdiff_t offsetS = classMemberDesc.GetOffset();

				for (int i = 0; i < array.size(); i++)
				{
					offsetS += array[i].GetOffset();
					ExecuteOnType(array[i], offsetS, functionToExecute);
				}

				return;
			}
		}
		else
		{
			// Unsupported input, can not be modified through FG
			functionToExecute(eFDT_Any, classMemberDesc, variableOffset);
		}
	}
};

CEntityComponentSignalFlowNode::CEntityComponentSignalFlowNode(const Schematyc::IEnvComponent & envComponent, const Schematyc::IEnvSignal & signal)
	: m_envComponent(envComponent)
	, m_envSignal(signal)
{
	m_inputs.push_back(InputPortConfig<int>("ComponentTypeIndex", 0, "Index of the component we want to receive the signal from, typically 0 - only useful if there are multiple components of the same type in the entity"));

	// Add an empty input port at the end. Apparently this is necessary to end the description.
	m_inputs.push_back(SInputPortConfig());

	m_outputs.push_back(OutputPortConfig_Void("ReceivedSignal", "Will be executed once the signal was received by the component."));

	const Schematyc::CClassMemberDescArray& memberDescArray = signal.GetDesc().GetMembers();

	m_outputs.reserve(memberDescArray.size());

	for (int i = 0; i < memberDescArray.size(); i++)
	{
		FlowGraphComponentUtils::ExecuteOnType(memberDescArray[i], memberDescArray[i].GetOffset(), [&](EFlowDataTypes type, const Schematyc::CClassMemberDesc& classDesc, ptrdiff_t offset)
		{
			SOutputPortConfig output;
			output.name = output.humanName = classDesc.GetName();
			output.description = classDesc.GetDescription();
			output.type = type;
			m_outputs.push_back(output);
		});
	}

	// Add an empty input port at the end. Apparently this is necessary to end the description.
	m_outputs.push_back(SOutputPortConfig());
}

CEntityComponentSignalFlowNode::~CEntityComponentSignalFlowNode()
{
	if (m_pCurrentEntity != nullptr)
	{
		CEntity* pEntity = reinterpret_cast<CEntity*>(m_pCurrentEntity);
		if (m_pCurrentEntity->GetSchematycObject() != nullptr)
		{
			pEntity->RemoveSchematycObject();
		}
	}
}

void CEntityComponentSignalFlowNode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
	// Set activation data because we will need it later to call the output ports
	m_activationInfo = *pActInfo;

	if (event == IFlowNode::eFE_SetEntityId && pActInfo->pEntity)
	{
		if (m_pCurrentEntity)
		{
			CEntity* pEntity = reinterpret_cast<CEntity*>(m_pCurrentEntity);

			// Make sure we clean up the previous entity
			if (pEntity->GetSchematycObject() && pEntity->GetId() != pActInfo->pEntity->GetId())
			{
				pEntity->RemoveSchematycObject();
			}
		}

		m_pCurrentEntity = pActInfo->pEntity;
		CreateSchematycEntity();
	}
}

void CEntityComponentSignalFlowNode::CreateSchematycEntity()
{
	CEntity* pEntity = reinterpret_cast<CEntity*>(m_pCurrentEntity);

	if (pEntity->GetSchematycObject() == nullptr)
	{
		// Add "fake" Schematyc object to the entity in order to receive signals
		SEntitySpawnParams params;
		params.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClassByGUID(FlowGraphComponentUtils::GetSignalEntityClassGUID());
		
		pEntity->CreateSchematycObject(params);
		
		pEntity->GetSchematycObject()->AddSignalListener(*this);
	}
}

void CEntityComponentSignalFlowNode::GetConfiguration(SFlowNodeConfig& config)
{
	config.nFlags |= EFLN_TARGET_ENTITY;
	config.sDescription = m_envComponent.GetDescription();
	config.pInputPorts = m_inputs.data();
	config.pOutputPorts = m_outputs.data();
	config.SetCategory(EFLN_APPROVED);
}

void CEntityComponentSignalFlowNode::ProcessSignal(const Schematyc::SObjectSignal& signal)
{
	if (m_envSignal.GetGUID() == signal.typeGUID)
	{
		DynArray<IEntityComponent*> components;
		m_pCurrentEntity->GetComponentsByTypeId(m_envComponent.GetGUID(), components);

		if (components.size() > m_currentComponentIndex)
		{
			IEntityComponent* pComponent = components[m_currentComponentIndex];

			if (signal.senderGUID == pComponent->GetGUID())
			{
				// Set signal received output. Every signal has this output.
				ActivateOutput(&m_activationInfo, static_cast<int>(FlowGraphComponentUtils::ESignalOutputPorts::Received), TFlowInputData());

				size_t outputIndex = 0;

				for (int i = 0; i < m_envSignal.GetDesc().GetMembers().size(); ++i)
				{
					const Schematyc::CClassMemberDesc& memberDesc = m_envSignal.GetDesc().GetMembers()[i];

					FlowGraphComponentUtils::ExecuteOnType(memberDesc, memberDesc.GetOffset(), [&](EFlowDataTypes type, const Schematyc::CClassMemberDesc& classDesc, ptrdiff_t offset)
					{
						switch (type)
						{
						case eFDT_EntityId:
						case eFDT_Int:
						{
							FlowGraphComponentUtils::ActivateSignalOutput<int>(&signal, &m_activationInfo, outputIndex);
						}
						break;
						case eFDT_Float:
						{
							FlowGraphComponentUtils::ActivateSignalOutput<float>(&signal, &m_activationInfo, outputIndex);
						}
						break;
						case eFDT_Vec3:
						{
							FlowGraphComponentUtils::ActivateSignalOutput<Vec3>(&signal, &m_activationInfo, outputIndex);
						}
						break;
						case eFDT_String:
						{
							FlowGraphComponentUtils::ActivateSignalOutput<string>(&signal, &m_activationInfo, outputIndex);
						}
						break;
						case eFDT_Bool:
						{
							FlowGraphComponentUtils::ActivateSignalOutput<bool>(&signal, &m_activationInfo, outputIndex);
						}
						break;
						}

						++outputIndex;
					});
				}
			}
		}
	}
}

CEntityComponentGetterFlowNode::CEntityComponentGetterFlowNode(const Schematyc::CClassMemberDescArray & memberDescArray, const Schematyc::IEnvComponent & component)
	: m_envComponent(component)
	, m_memberDescArray(memberDescArray)
{
	m_inputs.push_back(InputPortConfig_Void("Get Parameter", "Gets the parameter of the components"));
	m_inputs.push_back(InputPortConfig<int>("ComponentTypeIndex", 0, "Index of the component we want to activate, typically 0 - only useful if there are multiple components of the same type in the entity"));

	// Add an empty input port at the end. Apparently this is necessary to end the description.
	m_inputs.push_back(SInputPortConfig());

	m_outputs.reserve(memberDescArray.size());

	for (int i = 0; i < memberDescArray.size(); ++i)
	{
		FlowGraphComponentUtils::ExecuteOnType(memberDescArray[i], memberDescArray[i].GetOffset(), [&](EFlowDataTypes type, const Schematyc::CClassMemberDesc& classDesc, ptrdiff_t offset)
		{
			SOutputPortConfig output;
			output.name = output.humanName = classDesc.GetName();
			output.description = classDesc.GetDescription();
			output.type = type;
			m_outputs.push_back(output);
		});
	}

	// Add an empty input port at the end. Apparently this is necessary to end the description.
	m_outputs.push_back(SOutputPortConfig());
}

void CEntityComponentGetterFlowNode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
	switch (event)
	{
	case eFE_Activate:
	{
		// Check if the Call input was activated
		if (IsPortActive(pActInfo, static_cast<int>(FlowGraphComponentUtils::EInputPorts::Call)) && pActInfo->pEntity != nullptr)
		{
			DynArray<IEntityComponent*> components;
			pActInfo->pEntity->GetComponentsByTypeId(m_envComponent.GetGUID(), components);
			const int componentIndex = GetPortInt(pActInfo, static_cast<int>(FlowGraphComponentUtils::EInputPorts::ComponentIndex));

			size_t outputIndex = 0;

			if (componentIndex < components.size())
			{
				for (int i = 0; i < m_envComponent.GetDesc().GetMembers().size(); ++i)
				{
					IEntityComponent* pComponent = components[componentIndex];
					const Schematyc::CClassMemberDesc& memberDesc = m_envComponent.GetDesc().GetMembers()[i];

					FlowGraphComponentUtils::ExecuteOnType(memberDesc, memberDesc.GetOffset(), [&](EFlowDataTypes type, const Schematyc::CClassMemberDesc& classDesc, ptrdiff_t offset)
					{
						switch (type)
						{
						case eFDT_EntityId:
						case eFDT_Int:
						{
							FlowGraphComponentUtils::ActivateGetterOutput<int>(pComponent, classDesc, pActInfo, outputIndex, offset);
						}
						break;
						case eFDT_Float:
						{
							FlowGraphComponentUtils::ActivateGetterOutput<float>(pComponent, classDesc, pActInfo, outputIndex, offset);
						}
						break;
						case eFDT_Vec3:
						{
							FlowGraphComponentUtils::ActivateGetterOutput<Vec3>(pComponent, classDesc, pActInfo, outputIndex, offset);
						}
						break;
						case eFDT_String:
						{
							FlowGraphComponentUtils::ActivateGetterOutput<string>(pComponent, classDesc, pActInfo, outputIndex, offset);
						}
						break;
						case eFDT_Bool:
						{
							FlowGraphComponentUtils::ActivateGetterOutput<bool>(pComponent, classDesc, pActInfo, outputIndex, offset);
						}
						break;
						}

						++outputIndex;
					});
				}
			}
		}
	}
	break;
	};
}

void CEntityComponentGetterFlowNode::GetConfiguration(SFlowNodeConfig& config)
{
	config.nFlags |= EFLN_TARGET_ENTITY;
	config.sDescription = m_envComponent.GetDescription();
	config.pInputPorts = m_inputs.data();
	config.pOutputPorts = m_outputs.data();
	config.SetCategory(EFLN_APPROVED);
}

CEntityComponentSetterFlowNode::CEntityComponentSetterFlowNode(const Schematyc::CClassMemberDescArray& memberDescArray, const Schematyc::IEnvComponent& component)
	: m_envComponent(component)
	, m_memberDescArray(memberDescArray)
{
	m_inputs.push_back(InputPortConfig_Void("Set Parameter", "Sets the parameter of the components"));
	m_inputs.push_back(InputPortConfig<int>("ComponentTypeIndex", 0, "Index of the component we want to activate, typically 0 - only useful if there are multiple components of the same type in the entity"));
	m_inputs.reserve(m_memberDescArray.size());

	for (int i = 0; i < memberDescArray.size(); ++i)
	{
		FlowGraphComponentUtils::ExecuteOnType(memberDescArray[i], memberDescArray[i].GetOffset(), [&](EFlowDataTypes type, const Schematyc::CClassMemberDesc& classDesc, ptrdiff_t offset)
		{
			SInputPortConfig input;
			input.name = input.humanName = classDesc.GetName();
			input.description = classDesc.GetDescription();
			input.sUIConfig = nullptr;

			switch (type)
			{
			case eFDT_Bool:
				input.defaultData = TFlowInputData(*static_cast<const bool*>(classDesc.GetDefaultValue()), true);
				break;
			case eFDT_EntityId:
				input.defaultData = TFlowInputData(*static_cast<const EntityId*>(classDesc.GetDefaultValue()), true);
				break;
			case eFDT_String:
			{
				const void* pDefaultValue = classDesc.GetTypeDesc().GetDefaultValue();
				if (pDefaultValue != nullptr)
				{
					const Schematyc::CSharedString* pSharedString = static_cast<const Schematyc::CSharedString*>(pDefaultValue);
					input.defaultData = TFlowInputData(string(pSharedString->c_str()), true);
				}
			}
			break;
			case eFDT_Int:
				input.defaultData = TFlowInputData(*static_cast<const int*>(classDesc.GetDefaultValue()), true);
				break;
			case eFDT_Float:
				input.defaultData = TFlowInputData(*static_cast<const float*>(classDesc.GetDefaultValue()), true);
				break;
			case eFDT_Vec3:
				input.defaultData = TFlowInputData(*static_cast<const Vec3*>(classDesc.GetDefaultValue()), true);
				break;
			case eFDT_Any:
				input.defaultData = TFlowInputData(SFlowSystemVoid(), false);
				break;
			default:
				return;
				break;
			}

			m_inputs.push_back(input);
		});
	}

	// Add an empty input and out port at the end. Apparently this is necessary to end the description.
	m_inputs.push_back(SInputPortConfig());
}

void CEntityComponentSetterFlowNode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
	switch (event)
	{
	case eFE_Activate:
	{
		// Check if the Call input was activated
		if (IsPortActive(pActInfo, static_cast<int>(FlowGraphComponentUtils::EInputPorts::Call)) && pActInfo->pEntity != nullptr)
		{
			DynArray<IEntityComponent*> components;
			pActInfo->pEntity->GetComponentsByTypeId(m_envComponent.GetGUID(), components);
			const int componentIndex = GetPortInt(pActInfo, static_cast<int>(FlowGraphComponentUtils::EInputPorts::ComponentIndex));

			size_t flowgraphInputIndex = static_cast<size_t>(FlowGraphComponentUtils::EInputPorts::DefaultPortCount);
			if (componentIndex < components.size())
			{
				size_t inputIndex = 0;

				for (int i = 0; i < m_envComponent.GetDesc().GetMembers().size(); ++i, ++flowgraphInputIndex)
				{
					IEntityComponent* pComponent = components[componentIndex];
					const Schematyc::CClassMemberDesc& memberDesc = m_envComponent.GetDesc().GetMembers()[i];

					FlowGraphComponentUtils::ExecuteOnType(memberDesc, memberDesc.GetOffset(), [&](EFlowDataTypes type, const Schematyc::CClassMemberDesc& classDesc, ptrdiff_t offset)
					{
						switch (type)
						{
						case eFDT_EntityId:
						case eFDT_Int:
						{
							const int value = GetPortInt(pActInfo, flowgraphInputIndex);
							FlowGraphComponentUtils::SetComponentValue(pComponent, inputIndex, value, classDesc, offset);
						}
						break;
						case eFDT_Float:
						{
							const float value = GetPortFloat(pActInfo, flowgraphInputIndex);
							FlowGraphComponentUtils::SetComponentValue(pComponent, inputIndex, value, classDesc, offset);
						}
						break;
						case eFDT_Vec3:
						{
							const Vec3 value = GetPortVec3(pActInfo, flowgraphInputIndex);
							FlowGraphComponentUtils::SetComponentValue(pComponent, inputIndex, value, classDesc, offset);
						}
						break;
						case eFDT_String:
						{
							const string value = GetPortString(pActInfo, flowgraphInputIndex);
							FlowGraphComponentUtils::SetComponentValue(pComponent, inputIndex, value, classDesc, offset);
						}
						break;
						case eFDT_Bool:
						{
							const bool value = GetPortBool(pActInfo, flowgraphInputIndex);
							FlowGraphComponentUtils::SetComponentValue(pComponent, inputIndex, value, classDesc, offset);
						}
						break;
						}

						++inputIndex;
					});
				}
			}
		}
	}
	break;
	};
}

void CEntityComponentSetterFlowNode::GetConfiguration(SFlowNodeConfig& config)
{
	static const SOutputPortConfig outputPorts[] =
	{
		{ 0 }
	};

	config.nFlags |= EFLN_TARGET_ENTITY;
	config.sDescription = m_envComponent.GetDescription();
	config.pInputPorts = m_inputs.data();
	config.pOutputPorts = outputPorts;
	config.SetCategory(EFLN_APPROVED);
}

CEntityComponentFunctionFlowNode::CEntityComponentFunctionFlowNode(SActivationInfo* pActInfo, const Schematyc::IEnvComponent& component, const Schematyc::CEnvFunction& function)
	: m_envComponent(component)
	, m_envFunction(function)
{
	const int parameterCount = m_envFunction.GetInputCount();

	m_inputs.reserve(static_cast<size_t>(FlowGraphComponentUtils::EInputPorts::DefaultPortCount) + parameterCount + 1);
	m_inputs.push_back(InputPortConfig_Void("Call", "Calls the function"));
	m_inputs.push_back(InputPortConfig<int>("ComponentTypeIndex", 0, "Index of the component we want to activate, typically 0 - only useful if there are multiple components of the same type in the entity"));

	for (int parameterIndex = 0; parameterIndex < parameterCount; ++parameterIndex)
	{
		SInputPortConfig input;

		input.name = input.humanName = m_envFunction.GetInputName(parameterIndex);
		input.description = m_envFunction.GetInputDescription(parameterIndex);

		Schematyc::CAnyConstPtr defaultInputData = m_envFunction.GetInputData(parameterIndex);
		const Schematyc::CCommonTypeDesc& inputTypeDescription = defaultInputData->GetTypeDesc();

		if (inputTypeDescription.GetGUID() == Schematyc::GetTypeGUID<int>())
		{
			input.defaultData = TFlowInputData(*static_cast<const int*>(defaultInputData->GetValue()), true);
		}
		else if (inputTypeDescription.GetGUID() == Schematyc::GetTypeGUID<float>())
		{
			input.defaultData = TFlowInputData(*static_cast<const float*>(defaultInputData->GetValue()), true);
		}
		else if (inputTypeDescription.GetGUID() == Schematyc::GetTypeGUID<Schematyc::ExplicitEntityId>())
		{
			input.defaultData = TFlowInputData(*static_cast<const EntityId*>(defaultInputData->GetValue()), true);
		}
		else if (inputTypeDescription.GetGUID() == Schematyc::GetTypeGUID<Vec3>())
		{
			input.defaultData = TFlowInputData(*static_cast<const Vec3*>(defaultInputData->GetValue()), true);
		}
		else if (inputTypeDescription.GetGUID() == Schematyc::GetTypeGUID<Schematyc::CSharedString>())
		{
			const Schematyc::CSharedString* pSharedString = static_cast<const Schematyc::CSharedString*>(defaultInputData->GetValue());

			input.defaultData = TFlowInputData(string(pSharedString->c_str()), true);
		}
		else if (inputTypeDescription.GetGUID() == Schematyc::GetTypeGUID<bool>())
		{
			input.defaultData = TFlowInputData(*static_cast<const bool*>(defaultInputData->GetValue()), true);
		}
		else
		{
			// Unsupported input, can not be modified through FG
			input.defaultData = TFlowInputData(SFlowSystemVoid(), false);
		}

		// TODO: Support Enum
		input.sUIConfig = nullptr;

		m_inputs.push_back(input);
	}

	m_inputs.push_back(SInputPortConfig());
}

void CEntityComponentFunctionFlowNode::GetConfiguration(SFlowNodeConfig& config)
{
	static const SOutputPortConfig outputPorts[] =
	{
		{ 0 }
	};

	config.nFlags |= EFLN_TARGET_ENTITY;
	config.sDescription = m_envComponent.GetDescription();
	config.pInputPorts = m_inputs.data();
	config.pOutputPorts = outputPorts;
	config.SetCategory(EFLN_APPROVED);
}

void CEntityComponentFunctionFlowNode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
	switch (event)
	{
	case eFE_Activate:
	{
		// Check if the Call input was activated
		if (IsPortActive(pActInfo, static_cast<int>(FlowGraphComponentUtils::EInputPorts::Call)) && pActInfo->pEntity != nullptr)
		{
			DynArray<IEntityComponent*> components;
			pActInfo->pEntity->GetComponentsByTypeId(m_envComponent.GetGUID(), components);
			const int componentIndex = GetPortInt(pActInfo, static_cast<int>(FlowGraphComponentUtils::EInputPorts::ComponentIndex));
			if (componentIndex < components.size())
			{
				const int numSchematycInputs = m_inputs.size() - static_cast<int>(FlowGraphComponentUtils::EInputPorts::DefaultPortCount) - 1;

				IEntityComponent* pComponent = components[componentIndex];

				Schematyc::CRuntimeParamMap params;
				params.ReserveInputs(numSchematycInputs + 1);

				const Schematyc::CAnyValuePtr pReturnValue = Schematyc::CAnyValue::MakeShared(true);
				// Bind return value
				params.BindInput(Schematyc::CUniqueId::FromIdx(0), pReturnValue);

				std::vector<Schematyc::CAnyValuePtr> arguments;
				arguments.resize(numSchematycInputs);

				for (int i = 0; i < numSchematycInputs; ++i)
				{
					const int flowgraphInputIndex = static_cast<int>(FlowGraphComponentUtils::EInputPorts::DefaultPortCount) + i;

					const Schematyc::CUniqueId uniqueInputId = Schematyc::CUniqueId::FromUInt32(m_envFunction.GetInputId(i));

					switch (m_inputs[flowgraphInputIndex].defaultData.GetType())
					{
					case EFlowDataTypes::eFDT_Int:
					{
						const int value = GetPortInt(pActInfo, flowgraphInputIndex);
						arguments[i] = Schematyc::CAnyValue::MakeShared(value);

						params.BindInput(uniqueInputId, arguments[i]);
					}
					break;
					case EFlowDataTypes::eFDT_Float:
					{
						const float value = GetPortFloat(pActInfo, flowgraphInputIndex);
						arguments[i] = Schematyc::CAnyValue::MakeShared(value);

						params.BindInput(uniqueInputId, arguments[i]);
					}
					break;
					case EFlowDataTypes::eFDT_EntityId:
					{
						const Schematyc::ExplicitEntityId value = static_cast<Schematyc::ExplicitEntityId>(GetPortEntityId(pActInfo, flowgraphInputIndex));
						arguments[i] = Schematyc::CAnyValue::MakeShared(value);

						params.BindInput(uniqueInputId, arguments[i]);
					}
					break;
					case EFlowDataTypes::eFDT_Vec3:
					{
						const Vec3 value = GetPortVec3(pActInfo, flowgraphInputIndex);
						arguments[i] = Schematyc::CAnyValue::MakeShared(value);

						params.BindInput(uniqueInputId, arguments[i]);
					}
					break;
					case EFlowDataTypes::eFDT_String:
					{
						const Schematyc::CSharedString value = GetPortString(pActInfo, flowgraphInputIndex);
						arguments[i] = Schematyc::CAnyValue::MakeShared(value);

						params.BindInput(uniqueInputId, arguments[i]);
					}
					break;
					case EFlowDataTypes::eFDT_Bool:
					{
						const bool value = GetPortBool(pActInfo, flowgraphInputIndex);
						arguments[i] = Schematyc::CAnyValue::MakeShared(value);

						params.BindInput(uniqueInputId, arguments[i]);
					}
					break;
					default:
					{
						// Fall back to default value
						params.BindInput(uniqueInputId, m_envFunction.GetInputData(i));
					}
					break;
					}
				}

				m_envFunction.Execute(params, pComponent);
			}
		}
	}
	break;
	};
}

