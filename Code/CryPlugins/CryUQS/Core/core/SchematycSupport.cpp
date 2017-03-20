// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

//#pragma  optimize("", off)

#if UQS_SCHEMATYC_SUPPORT

namespace UQS
{
	namespace Core
	{

		//===================================================================================
		//
		// CSchematycEnvDataTypeFinder
		//
		//===================================================================================

		CSchematycEnvDataTypeFinder::CSchematycEnvDataTypeFinder(const Schematyc::CTypeName& typeNameToSearchFor)
			: m_typeNameToSearchFor(typeNameToSearchFor)
			, m_pFoundDataType(nullptr)
		{}

		Schematyc::EVisitStatus CSchematycEnvDataTypeFinder::OnVisitEnvDataType(const Schematyc::IEnvDataType& dataType)
		{
			const Schematyc::CTypeName& typeName = dataType.GetDesc().GetName();

			if (typeName == m_typeNameToSearchFor)
			{
				m_pFoundDataType = &dataType;
				return Schematyc::EVisitStatus::Stop;
			}
			else
			{
				return Schematyc::EVisitStatus::Continue;
			}
		}

		const Schematyc::IEnvDataType* CSchematycEnvDataTypeFinder::FindEnvDataTypeByTypeName(const Schematyc::CTypeName& typeNameToSearchFor)
		{
			CSchematycEnvDataTypeFinder finder(typeNameToSearchFor);
			Schematyc::EnvDataTypeConstVisitor visitor = Schematyc::EnvDataTypeConstVisitor::FromMemberFunction<CSchematycEnvDataTypeFinder, &CSchematycEnvDataTypeFinder::OnVisitEnvDataType>(finder);
			gEnv->pSchematyc->GetEnvRegistry().VisitDataTypes(visitor);
			return finder.m_pFoundDataType;
		}

		//===================================================================================
		//
		// CSchematycUqsComponentEnvFunctionBase
		//
		//===================================================================================

		CSchematycUqsComponentEnvFunctionBase::CSchematycUqsComponentEnvFunctionBase(const Schematyc::SSourceFileInfo& sourceFileInfo)
			: CEnvElementBase(sourceFileInfo)
		{
		}

		CSchematycUqsComponentEnvFunctionBase::~CSchematycUqsComponentEnvFunctionBase()
		{
		}

		bool CSchematycUqsComponentEnvFunctionBase::IsValidScope(Schematyc::IEnvElement& scope) const
		{
			switch (scope.GetType())
			{
			case Schematyc::EEnvElementType::Module:
			case Schematyc::EEnvElementType::DataType:
			case Schematyc::EEnvElementType::Class:
			case Schematyc::EEnvElementType::Component:
			case Schematyc::EEnvElementType::Action:
				return true;

			default:
				return false;
			}
		}

		bool CSchematycUqsComponentEnvFunctionBase::Validate() const
		{
			return true;
		}

		Schematyc::EnvFunctionFlags CSchematycUqsComponentEnvFunctionBase::GetFunctionFlags() const
		{
			return Schematyc::EEnvFunctionFlags::Member;
		}

		const Schematyc::CCommonTypeDesc* CSchematycUqsComponentEnvFunctionBase::GetObjectTypeDesc() const
		{
			return &Schematyc::GetTypeDesc<CSchematycUqsComponent>();
		}

		uint32 CSchematycUqsComponentEnvFunctionBase::GetInputCount() const
		{
			return m_functionBinding.inputCount;
		}

		uint32 CSchematycUqsComponentEnvFunctionBase::GetInputId(uint32 inputIdx) const
		{
			return inputIdx < m_functionBinding.inputCount ? m_functionBinding.params[m_functionBinding.inputs[inputIdx]].id : Schematyc::InvalidId;
		}

		const char* CSchematycUqsComponentEnvFunctionBase::GetInputName(uint32 inputIdx) const
		{
			return inputIdx < m_functionBinding.inputCount ? m_functionBinding.params[m_functionBinding.inputs[inputIdx]].name.c_str() : "";
		}

		const char* CSchematycUqsComponentEnvFunctionBase::GetInputDescription(uint32 inputIdx) const
		{
			return "";
		}

		Schematyc::CAnyConstPtr CSchematycUqsComponentEnvFunctionBase::GetInputData(uint32 inputIdx) const
		{
			return inputIdx < m_functionBinding.inputCount ? m_functionBinding.params[m_functionBinding.inputs[inputIdx]].pData.get() : nullptr;
		}

		uint32 CSchematycUqsComponentEnvFunctionBase::GetOutputCount() const
		{
			return m_functionBinding.outputCount;
		}

		uint32 CSchematycUqsComponentEnvFunctionBase::GetOutputId(uint32 outputIdx) const
		{
			return outputIdx < m_functionBinding.outputCount ? m_functionBinding.params[m_functionBinding.outputs[outputIdx]].id : Schematyc::InvalidId;
		}

		const char* CSchematycUqsComponentEnvFunctionBase::GetOutputName(uint32 outputIdx) const
		{
			return outputIdx < m_functionBinding.outputCount ? m_functionBinding.params[m_functionBinding.outputs[outputIdx]].name.c_str() : "";
		}

		const char* CSchematycUqsComponentEnvFunctionBase::GetOutputDescription(uint32 outputIdx) const
		{
			return "";
		}

		Schematyc::CAnyConstPtr CSchematycUqsComponentEnvFunctionBase::GetOutputData(uint32 outputIdx) const
		{
			return outputIdx < m_functionBinding.outputCount ? m_functionBinding.params[m_functionBinding.outputs[outputIdx]].pData.get() : nullptr;
		}

		void CSchematycUqsComponentEnvFunctionBase::Execute(Schematyc::CRuntimeParamMap& params, void* pObject) const
		{
			assert(pObject != nullptr);
			ExecuteOnSchematycUqsComponent(params, *static_cast<CSchematycUqsComponent*>(pObject));
		}

		bool CSchematycUqsComponentEnvFunctionBase::AreAllParamtersRecognized() const
		{
			// check if all parameters have a default value by now (if they don't, then this particular port won't show up in the graph)
			for (uint32 i = 0; i < m_functionBinding.totalParamsCount; ++i)
			{
				if (!m_functionBinding.params[i].pData)
					return false;
			}
			return true;
		}

		void CSchematycUqsComponentEnvFunctionBase::AddInputParamByTypeName(const char* szParamName, uint32 id, const Schematyc::CTypeName& typeName)
		{
			AddInputParam(szParamName, id, &typeName, nullptr);
		}

		void CSchematycUqsComponentEnvFunctionBase::AddInputParamByTypeDesc(const char* szParamName, uint32 id, const Schematyc::CCommonTypeDesc& typeDesc)
		{
			AddInputParam(szParamName, id, nullptr, &typeDesc);
		}

		void CSchematycUqsComponentEnvFunctionBase::AddInputParam(const char* szParamName, uint32 id, const Schematyc::CTypeName* pTypeName, const Schematyc::CCommonTypeDesc* pTypeDesc)
		{
			assert(m_functionBinding.totalParamsCount < Schematyc::EnvFunctionUtils::MaxParams);

			m_functionBinding.params[m_functionBinding.totalParamsCount] = CreateParam(szParamName, id, pTypeName, pTypeDesc, Schematyc::EnvFunctionUtils::EParamFlags::BoundToInput);
			m_functionBinding.inputs[m_functionBinding.inputCount++] = m_functionBinding.totalParamsCount++;
		}

		void CSchematycUqsComponentEnvFunctionBase::AddOutputParamByTypeName(const char* szParamName, uint32 id, const Schematyc::CTypeName& typeName)
		{
			AddOutputParam(szParamName, id, &typeName, nullptr);
		}

		void CSchematycUqsComponentEnvFunctionBase::AddOutputParamByTypeDesc(const char* szParamName, uint32 id, const Schematyc::CCommonTypeDesc& typeDesc)
		{
			AddOutputParam(szParamName, id, nullptr, &typeDesc);
		}

		void CSchematycUqsComponentEnvFunctionBase::AddOutputParam(const char* szParamName, uint32 id, const Schematyc::CTypeName* pTypeName, const Schematyc::CCommonTypeDesc* pTypeDesc)
		{
			assert(m_functionBinding.totalParamsCount < Schematyc::EnvFunctionUtils::MaxParams);

			m_functionBinding.params[m_functionBinding.totalParamsCount] = CreateParam(szParamName, id, pTypeName, pTypeDesc, Schematyc::EnvFunctionUtils::EParamFlags::BoundToOutput);
			m_functionBinding.outputs[m_functionBinding.outputCount++] = m_functionBinding.totalParamsCount++;
		}

		Schematyc::CAnyConstRef CSchematycUqsComponentEnvFunctionBase::GetUntypedInputParam(const Schematyc::CRuntimeParamMap& params, uint32 inputIdx) const
		{
			assert(inputIdx < m_functionBinding.inputCount);

			const uint32 paramIdx = m_functionBinding.inputs[inputIdx];
			const SMyParamBinding& paramBinding = m_functionBinding.params[paramIdx];
			const Schematyc::CUniqueId id = Schematyc::CUniqueId::FromUInt32(paramBinding.id);
			Schematyc::CAnyConstPtr ptr = params.GetInput(id);

			// if the user didn't connect the input port then use the default value provided by paramBinding
			if (!ptr)
			{
				ptr = paramBinding.pData;
				assert(ptr);
			}

			return *ptr;
		}

		Schematyc::CAnyRef CSchematycUqsComponentEnvFunctionBase::GetUntypedOutputParam(Schematyc::CRuntimeParamMap& params, uint32 outputIdx) const
		{
			assert(outputIdx < m_functionBinding.outputCount);

			const uint32 paramIdx = m_functionBinding.outputs[outputIdx];
			const SMyParamBinding& paramBinding = m_functionBinding.params[paramIdx];
			const Schematyc::CUniqueId id = Schematyc::CUniqueId::FromUInt32(paramBinding.id);
			Schematyc::CAnyPtr ptr = params.GetOutput(id);

			// if the user didn't connect the input port then use the default value provided by paramBinding
			if (!ptr)
			{
				ptr = paramBinding.pData;
				assert(ptr);
			}

			return *ptr;
		}

		stack_string CSchematycUqsComponentEnvFunctionBase::PatchName(const char* szName)
		{
			stack_string patchedName = szName;
			for (char* c = patchedName.begin(); *c != '\0'; ++c)
			{
				if (c[0] == ':' && c[1] == ':')
				{
					*(++c) = ' ';
				}
			}
			return patchedName;
		}

		CSchematycUqsComponentEnvFunctionBase::SMyParamBinding CSchematycUqsComponentEnvFunctionBase::CreateParam(const char* szParamName, uint32 id, const Schematyc::CTypeName* pTypeName, const Schematyc::CCommonTypeDesc* pTypeDesc, Schematyc::EnvFunctionUtils::EParamFlags paramFlags) const
		{
			assert(pTypeName || pTypeDesc);

			// TODO: ensure each param ends up with a unique id

			SMyParamBinding p;

			p.flags = paramFlags;
			p.idx = m_functionBinding.totalParamsCount;
			p.id = id;
			p.name = szParamName;

			// figure out the type descriptor by looking it up in the IEnvRegistry
			if (!pTypeDesc)
			{
				const Schematyc::IEnvDataType* pEnvDataType = CSchematycEnvDataTypeFinder::FindEnvDataTypeByTypeName(*pTypeName);

				// Note: it could be that we didn't find the corresponding IEnvDataType.
				//       This could be the case if the item type has no corresponding type on the schematyc side (e. g. CAIActor might only be known to UQS),
				//       or (and let's hope this is *not* case) that some modules haven't had yet a chance to register their data types to schematyc.
				//       So, if it's missing, we simply miss out on the default value - let's see how that behaves at edit- and runtime (UPDATE: the behavior is that this particular input port will be missing in the UI graph)
				if (pEnvDataType)
				{
					// fine
					pTypeDesc = &pEnvDataType->GetDesc();
				}
			}

			// provide a default value for the parameter
			if (pTypeDesc)
			{
				p.pData = Schematyc::CAnyValue::MakeSharedDefault(*pTypeDesc);
			}

			return p;
		}

		//===================================================================================
		//
		// CSchematycUqsComponentEnvFunction_AddParam
		//
		//===================================================================================

		CSchematycUqsComponentEnvFunction_AddParam::CSchematycUqsComponentEnvFunction_AddParam(const Schematyc::SSourceFileInfo& sourceFileInfo, Client::IItemFactory& itemFactory, const Client::IItemConverter* pOptionalFromForeignTypeConverter)
			: CSchematycUqsComponentEnvFunctionBase(sourceFileInfo)
			, m_itemFactory(itemFactory)
			, m_pFromForeignTypeConverter(pOptionalFromForeignTypeConverter)
		{
			const stack_string patchedItemFactoryName = PatchName(m_itemFactory.GetName());
			const Shared::CTypeInfo* pInputType = nullptr;

			if (m_pFromForeignTypeConverter)
			{
				SetName(stack_string().Format("AddParam [%s to %s]", PatchName(m_pFromForeignTypeConverter->GetFromName()).c_str(), patchedItemFactoryName.c_str()));
				SetGUID(m_pFromForeignTypeConverter->GetGUID());
				pInputType = &m_pFromForeignTypeConverter->GetFromItemType();
			}
			else
			{
				SetName(stack_string().Format("AddParam [%s]", patchedItemFactoryName.c_str()));
				SetGUID(m_itemFactory.GetGUIDForSchematycAddParamFunction());
				pInputType = &m_itemFactory.GetItemType();
			}

			// inputs
			AddInputParamByTypeDesc("Name", 'name', Schematyc::GetTypeDesc<Schematyc::CSharedString>());
			AddInputParamByTypeName("Value", 'valu', pInputType->GetSchematycTypeName());
		}

		void CSchematycUqsComponentEnvFunction_AddParam::GenerateAddParamFunctionsInRegistrationScope(Schematyc::CEnvRegistrationScope& scope, Client::IItemFactory& itemFactory)
		{
			// generate the default "AddParam" function that expects the same type as given item factory produces (i.e. item type conversion will *not* take place)
			{
				std::shared_ptr<CSchematycUqsComponentEnvFunction_AddParam> pAddParamFunction(new CSchematycUqsComponentEnvFunction_AddParam(SCHEMATYC_SOURCE_FILE_INFO, itemFactory, nullptr));
				if (pAddParamFunction->AreAllParamtersRecognized())
				{
					scope.Register(pAddParamFunction);
				}
			}

			// generate "AddParam" functions that *do* deal with type conversions from a foreign type
			{
				const Client::IItemConverterCollection& fromForeignTypeConverters = itemFactory.GetFromForeignTypeConverters();
				for (size_t i = 0, n = fromForeignTypeConverters.GetItemConverterCount(); i < n; ++i)
				{
					const Client::IItemConverter& fromForeignTypeConverter = fromForeignTypeConverters.GetItemConverter(i);
					std::shared_ptr<CSchematycUqsComponentEnvFunction_AddParam> pAddParamFunction(new CSchematycUqsComponentEnvFunction_AddParam(SCHEMATYC_SOURCE_FILE_INFO, itemFactory, &fromForeignTypeConverter));
					if (pAddParamFunction->AreAllParamtersRecognized())
					{
						scope.Register(pAddParamFunction);
					}
				}
			}
		}

		void CSchematycUqsComponentEnvFunction_AddParam::ExecuteOnSchematycUqsComponent(Schematyc::CRuntimeParamMap& params, CSchematycUqsComponent& schematycEntityUqsComponent) const
		{
			Core::IHub* pHub = Core::IHubPlugin::GetHubPtr();

			if (!pHub)
			{
				SCHEMATYC_ENV_WARNING("CSchematycUqsComponentEnvFunction_AddParam::ExecuteOnSchematycUqsComponent: UQS system is not available");
				return;
			}

			// inputs
			const Schematyc::CSharedString& name = GetTypedInputParam<Schematyc::CSharedString>(params, 0);
			Schematyc::CAnyConstRef value = GetUntypedInputParam(params, 1);

			Shared::IVariantDict& runtimeParams = schematycEntityUqsComponent.GetRuntimeParamsStorageOfUpcomingQuery();

			if (m_pFromForeignTypeConverter)
			{
				void* pConvertedItem = m_itemFactory.CreateItems(1, Client::IItemFactory::EItemInitMode::UseDefaultConstructor);
				m_pFromForeignTypeConverter->ConvertItem(value.GetValue(), pConvertedItem);
				runtimeParams.AddOrReplace(name.c_str(), m_itemFactory, pConvertedItem);
				m_itemFactory.DestroyItems(pConvertedItem);
			}
			else
			{
				runtimeParams.AddOrReplace(name.c_str(), m_itemFactory, value.GetValue());
			}
		}

		//===================================================================================
		//
		// CSchematycUqsComponentEnvFunction_GetItemFromResultSet
		//
		//===================================================================================

		CSchematycUqsComponentEnvFunction_GetItemFromResultSet::CSchematycUqsComponentEnvFunction_GetItemFromResultSet(const Schematyc::SSourceFileInfo& sourceFileInfo, Client::IItemFactory& itemFactory, const Client::IItemConverter* pOptionalToForeignTypeConverter)
			: CSchematycUqsComponentEnvFunctionBase(sourceFileInfo)
			, m_itemFactory(itemFactory)
			, m_pToForeignTypeConverter(pOptionalToForeignTypeConverter)
		{
			const stack_string patchedItemFactoryName = PatchName(m_itemFactory.GetName());
			const Shared::CTypeInfo* pOutputType = nullptr;

			if (m_pToForeignTypeConverter)
			{
				SetName(stack_string().Format("GetItemFromResultSet [%s to %s]", patchedItemFactoryName.c_str(), PatchName(m_pToForeignTypeConverter->GetToName()).c_str()));
				SetGUID(m_pToForeignTypeConverter->GetGUID());
				pOutputType = &m_pToForeignTypeConverter->GetToItemType();
			}
			else
			{
				SetName(stack_string().Format("GetItemFromResultSet [%s]", patchedItemFactoryName.c_str()));
				SetGUID(m_itemFactory.GetGUIDForSchematycGetItemFromResultSetFunction());
				pOutputType = &m_itemFactory.GetItemType();
			}

			// inputs
			AddInputParamByTypeDesc("QueryID", 'quid', Schematyc::GetTypeDesc<CSchematycUqsComponent::SQueryIdWrapper>());
			AddInputParamByTypeDesc("ResultIdx", 'ridx', Schematyc::GetTypeDesc<int32>());

			// outputs
			AddOutputParamByTypeName("Item", 'item', pOutputType->GetSchematycTypeName());
			AddOutputParamByTypeDesc("Score", 'scor', Schematyc::GetTypeDesc<float>());
		}

		void CSchematycUqsComponentEnvFunction_GetItemFromResultSet::GenerateGetItemFromResultSetFunctionsInRegistrationScope(Schematyc::CEnvRegistrationScope& scope, Client::IItemFactory& itemFactory)
		{
			// generate the default "GetItemFromResultSet" function outputs the same type as given item factory produces (i.e. item type conversion will *not* take place)
			{
				std::shared_ptr<CSchematycUqsComponentEnvFunction_GetItemFromResultSet> pGetItemFromResultSetFunction(new CSchematycUqsComponentEnvFunction_GetItemFromResultSet(SCHEMATYC_SOURCE_FILE_INFO, itemFactory, nullptr));
				if (pGetItemFromResultSetFunction->AreAllParamtersRecognized())
				{
					scope.Register(pGetItemFromResultSetFunction);
				}
			}

			// generate "GetItemFromResultSet" functions that *do* deal with type conversions to a foreign type
			{
				const Client::IItemConverterCollection& toForeignTypeConverters = itemFactory.GetToForeignTypeConverters();
				for (size_t i = 0, n = toForeignTypeConverters.GetItemConverterCount(); i < n; ++i)
				{
					const Client::IItemConverter& toForeignTypeConverter = toForeignTypeConverters.GetItemConverter(i);
					std::shared_ptr<CSchematycUqsComponentEnvFunction_GetItemFromResultSet> pGetItemFromResultSetFunction(new CSchematycUqsComponentEnvFunction_GetItemFromResultSet(SCHEMATYC_SOURCE_FILE_INFO, itemFactory, &toForeignTypeConverter));
					if (pGetItemFromResultSetFunction->AreAllParamtersRecognized())
					{
						scope.Register(pGetItemFromResultSetFunction);
					}
				}
			}
		}

		void CSchematycUqsComponentEnvFunction_GetItemFromResultSet::ExecuteOnSchematycUqsComponent(Schematyc::CRuntimeParamMap& params, CSchematycUqsComponent& schematycEntityUqsComponent) const
		{
			Core::IHub* pHub = Core::IHubPlugin::GetHubPtr();

			if (!pHub)
			{
				SCHEMATYC_ENV_WARNING("CSchematycUqsComponentEnvFunction_GetItemFromResultSet::ExecuteOnSchematycUqsComponent: UQS system is not available");
				return;
			}

			// inputs
			const CSchematycUqsComponent::SQueryIdWrapper& queryId = GetTypedInputParam<CSchematycUqsComponent::SQueryIdWrapper>(params, 0);
			const int32 resultIdx = GetTypedInputParam<int32>(params, 1);

			// outputs
			Schematyc::CAnyRef item = GetUntypedOutputParam(params, 0);
			float& score = GetTypedOutputParam<float>(params, 1);

			const Core::IQueryResultSet* pQueryResultSet = schematycEntityUqsComponent.GetQueryResultSet(queryId.queryId);

			if (!pQueryResultSet)
			{
				Shared::CUqsString queryIdAsString;
				queryId.queryId.ToString(queryIdAsString);
				SCHEMATYC_ENV_WARNING("CSchematycUqsComponentEnvFunction_GetItemFromResultSet::ExecuteOnSchematycUqsComponent: unknown queryID: %s", queryIdAsString.c_str());
				return;
			}

			if (resultIdx < 0 || resultIdx >= (int32)pQueryResultSet->GetResultCount())
			{
				SCHEMATYC_ENV_WARNING("CSchematycUqsComponentEnvFunction_GetItemFromResultSet::ExecuteOnSchematycUqsComponent: resultIdx out of range (should be [0..%i])", (int)pQueryResultSet->GetResultCount() - 1);
				return;
			}

			Client::IItemFactory& itemFactoryOfResultSet = pQueryResultSet->GetItemFactory();
			const Shared::CTypeInfo& outputType = m_pToForeignTypeConverter ? m_pToForeignTypeConverter->GetToItemType() : itemFactoryOfResultSet.GetItemType();

			if (item.GetTypeDesc().GetName() != outputType.GetSchematycTypeName())
			{
				SCHEMATYC_ENV_WARNING("CSchematycUqsComponentEnvFunction_GetItemFromResultSet::ExecuteOnSchematycUqsComponent: item type mismatch: type of resulting item is '%s', but it was tried to retrieve a '%s'", outputType.GetSchematycTypeName().c_str(), item.GetTypeDesc().GetName().c_str());
				return;
			}

			const Core::IQueryResultSet::SResultSetEntry entry = pQueryResultSet->GetResult((size_t)resultIdx);

			// write outputs
			score = entry.score;
			if (m_pToForeignTypeConverter)
			{
				m_pToForeignTypeConverter->ConvertItem(entry.pItem, item.GetValue());
			}
			else
			{
				itemFactoryOfResultSet.CopyItem(item.GetValue(), entry.pItem);
			}
		}

		//===================================================================================
		//
		// CSchematycUqsComponent::SQueryIdWrapper
		//
		//===================================================================================

		CSchematycUqsComponent::SQueryIdWrapper::SQueryIdWrapper()
			: queryId(CQueryID::CreateInvalid())
		{
		}

		CSchematycUqsComponent::SQueryIdWrapper::SQueryIdWrapper(const CQueryID& id)
			: queryId(id)
		{
		}

		bool CSchematycUqsComponent::SQueryIdWrapper::operator==(const SQueryIdWrapper& other) const
		{
			return this->queryId == other.queryId;
		}

		bool CSchematycUqsComponent::SQueryIdWrapper::Serialize(Serialization::IArchive& archive, const char* szName, const char* szLabel)
		{
			return archive(this->queryId, szName, szLabel);
		}

		void CSchematycUqsComponent::SQueryIdWrapper::ReflectType(Schematyc::CTypeDesc<SQueryIdWrapper>& desc)
		{
			desc.SetGUID("b0c4a5a2-8a6e-4a4f-b7c1-90bfb9d1898b"_schematyc_guid);
			desc.SetLabel("QueryID");
			desc.SetDescription("Handle of a running query.");
			desc.SetToStringOperator<&ToString>();
		}

		void CSchematycUqsComponent::SQueryIdWrapper::ToString(Schematyc::IString& output, const SQueryIdWrapper& input)
		{
			Shared::CUqsString tmp;
			input.queryId.ToString(tmp);
			output.assign(tmp.c_str());
		}

		void CSchematycUqsComponent::SQueryIdWrapper::Register(Schematyc::CEnvRegistrationScope& scope)
		{
			scope.Register(SCHEMATYC_MAKE_ENV_DATA_TYPE(SQueryIdWrapper));

			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&SchematycFunction_Equal, "ae7a5b0a-e3f4-4cd7-b01a-bbad7c7fe11e"_schematyc_guid, "Equal");
				pFunction->BindOutput(0, 'resu', "Result");
				pFunction->BindInput(1, 'qid1', "QueryID 1");
				pFunction->BindInput(2, 'qid2', "QueryID 2");
				scope.Register(pFunction);
			}
		}

		bool CSchematycUqsComponent::SQueryIdWrapper::SchematycFunction_Equal(const SQueryIdWrapper& queryId1, const SQueryIdWrapper& queryId2)
		{
			return queryId1.queryId == queryId2.queryId;
		}

		//===================================================================================
		//
		// SQueryFinishedSignal
		//
		//===================================================================================

		CSchematycUqsComponent::SQueryFinishedSignal::SQueryFinishedSignal()
			: queryId(CQueryID::CreateInvalid())
			, resultCount(0)
		{
			// nothing
		}

		CSchematycUqsComponent::SQueryFinishedSignal::SQueryFinishedSignal(const SQueryIdWrapper& _queryId, int32 _resultCount)
			: queryId(_queryId)
			, resultCount(_resultCount)
		{
			// nothing
		}

		void CSchematycUqsComponent::SQueryFinishedSignal::ReflectType(Schematyc::CTypeDesc<SQueryFinishedSignal>& desc)
		{
			desc.SetGUID("babd0a8b-2f84-4019-b6f5-0758ac62ec56"_schematyc_guid);
			desc.SetLabel("QueryFinished");
			desc.SetDescription("Sent when a query finishes without having run into an exception.");
			desc.AddMember(&SQueryFinishedSignal::queryId, 'quid', "queryId", "QueryID", nullptr);
			desc.AddMember(&SQueryFinishedSignal::resultCount, 'resc', "resultCount", "ResultCount", nullptr);
		}

		//===================================================================================
		//
		// SQueryExceptionSignal
		//
		//===================================================================================

		CSchematycUqsComponent::SQueryExceptionSignal::SQueryExceptionSignal()
			: queryId(CQueryID::CreateInvalid())
			, exceptionMessage()
		{
			// nothing
		}

		CSchematycUqsComponent::SQueryExceptionSignal::SQueryExceptionSignal(const SQueryIdWrapper& _queryId, const Schematyc::CSharedString& _exceptionMessage)
			: queryId(_queryId)
			, exceptionMessage(_exceptionMessage)
		{
			// nothing
		}

		void CSchematycUqsComponent::SQueryExceptionSignal::ReflectType(Schematyc::CTypeDesc<SQueryExceptionSignal>& desc)
		{
			desc.SetGUID("5d1ea803-63bb-436d-8044-252a150b916f"_schematyc_guid);
			desc.SetLabel("QueryException");
			desc.SetDescription("Sent when a query runs into an excpetion before finishing.");
			desc.AddMember(&SQueryExceptionSignal::queryId, 'quid', "queryId", "QueryID", nullptr);
			desc.AddMember(&SQueryExceptionSignal::exceptionMessage, 'exms', "exceptionMessage", "ExceptionMessage", nullptr);
		}

		//===================================================================================
		//
		// CSchematycUqsComponent::SUpcomingQueryInfo
		//
		//===================================================================================

		void CSchematycUqsComponent::SUpcomingQueryInfo::Clear()
		{
			this->queryBlueprintID = CQueryBlueprintID();
			this->querierName.clear();
			this->runtimeParams.Clear();
		}

		//===================================================================================
		//
		// CSchematycUqsComponent
		//
		//===================================================================================

		CSchematycUqsComponent::CSchematycUqsComponent()
		{
		}

		CSchematycUqsComponent::~CSchematycUqsComponent()
		{
			CancelRunningQueriesAndClearFinishedOnes();
		}

		void CSchematycUqsComponent::Run(Schematyc::ESimulationMode simulationMode)
		{
			CancelRunningQueriesAndClearFinishedOnes();

			switch (simulationMode)
			{
			case Schematyc::ESimulationMode::Game:
				{
					Schematyc::SUpdateParams updateParams(SCHEMATYC_MEMBER_DELEGATE(&CSchematycUqsComponent::Update, *this), m_connectionScope);
					gEnv->pSchematyc->GetUpdateScheduler().Connect(updateParams);
					break;
				}
			}
		}

		void CSchematycUqsComponent::Shutdown()
		{
			CancelRunningQueriesAndClearFinishedOnes();
		}

		Shared::CVariantDict& CSchematycUqsComponent::GetRuntimeParamsStorageOfUpcomingQuery()
		{
			return m_upcomingQueryInfo.runtimeParams;
		}

		const IQueryResultSet* CSchematycUqsComponent::GetQueryResultSet(const CQueryID& queryID) const
		{
			auto it = m_succeededQueries.find(queryID);
			return (it == m_succeededQueries.cend()) ? nullptr : it->second.pQueryResultSet.get();
		}

		void CSchematycUqsComponent::ReflectType(Schematyc::CTypeDesc<CSchematycUqsComponent>& desc)
		{
			desc.SetGUID("1d00010d-5b91-4b56-8cdd-52c8d966acb8"_schematyc_guid);
			desc.SetLabel("UQS");
			desc.SetDescription("Universal Query System component");
			//desc.SetIcon("icons:schematyc/entity_audio_component.ico");
			desc.SetComponentFlags({ Schematyc::EComponentFlags::Singleton });
		}

		void CSchematycUqsComponent::Register(Schematyc::IEnvRegistrar& registrar)
		{
			Schematyc::CEnvRegistrationScope entityScope = registrar.Scope(Schematyc::g_entityClassGUID);
			{
				Schematyc::CEnvRegistrationScope componentScope = entityScope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CSchematycUqsComponent));
				{
					//
					// data types
					//

					SQueryIdWrapper::Register(componentScope);

					//
					// functions
					//

					{

						//
						// generate an "AddParam" and "GetItemFromResultSet" functions for each UQS item type
						//

						{
							IItemFactoryDatabase& itemFactoryDB = g_hubImpl->GetItemFactoryDatabase();

							for (size_t i = 0, n = itemFactoryDB.GetFactoryCount(); i < n; ++i)
							{
								Client::IItemFactory& itemFactory = itemFactoryDB.GetFactory(i);

								// skip item factories with no GUID
								if (itemFactory.GetGUIDForSchematycAddParamFunction() == CryGUID::Null() || itemFactory.GetGUIDForSchematycGetItemFromResultSetFunction() == CryGUID::Null())
									continue;

								CSchematycUqsComponentEnvFunction_AddParam::GenerateAddParamFunctionsInRegistrationScope(componentScope, itemFactory);
								CSchematycUqsComponentEnvFunction_GetItemFromResultSet::GenerateGetItemFromResultSetFunctionsInRegistrationScope(componentScope, itemFactory);
							}
						}

						//
						// BeginQuery()
						//

						{
							std::shared_ptr<Schematyc::CEnvFunction> pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycUqsComponent::SchematycFunction_BeginQuery, "5ebcdda0-31f5-4e2d-afef-63fc0368cf9c"_schematyc_guid, "BeginQuery");
							pFunction->SetDescription("Prepares for starting a query");
							pFunction->BindInput(1, 'qbpn', "QueryBlueprint", "Name of the query blueprint that shall be started.");
							pFunction->BindInput(2, 'qunm', "QuerierName", "For debugging purpose: name of the querier to identify more easily in the query history.");
							componentScope.Register(pFunction);
						}

						//
						// StartQuery()
						//

						{
							std::shared_ptr<Schematyc::CEnvFunction> pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycUqsComponent::SchematycFunction_StartQuery, "f7205181-41cb-478d-a773-0d33a3805a99"_schematyc_guid, "StartQuery");
							pFunction->SetDescription("Starts the query that was previously prepared");
							pFunction->BindOutput(1, 'quid', "QueryID");
							componentScope.Register(pFunction);
						}
					}

					//
					// signals
					//

					componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(SQueryFinishedSignal));
					componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(SQueryExceptionSignal));

				}
			}
		}

		void CSchematycUqsComponent::SchematycFunction_BeginQuery(const Schematyc::CSharedString& queryBlueprintName, const Schematyc::CSharedString& querierName)
		{
			m_upcomingQueryInfo.Clear();

			IHub* pHub = IHubPlugin::GetHubPtr();

			if (!pHub)
			{
				SCHEMATYC_ENV_WARNING("CSchematycUqsComponent::SchematycFunction_BeginQuery: UQS system is not available");
				return;
			}

			const CQueryBlueprintID queryBlueprintID = pHub->GetQueryBlueprintLibrary().FindQueryBlueprintIDByName(queryBlueprintName.c_str());

			if (!queryBlueprintID.IsOrHasBeenValid())
			{
				SCHEMATYC_ENV_WARNING("CSchematycUqsComponent::SchematycFunction_BeginQuery: unknown or broken query blueprint: '%s'", queryBlueprintName.c_str());
				return;
			}

			m_upcomingQueryInfo.queryBlueprintID = queryBlueprintID;
			m_upcomingQueryInfo.querierName = querierName;
		}

		void CSchematycUqsComponent::SchematycFunction_StartQuery(SQueryIdWrapper& outQueryId)
		{
			IHub* pHub = IHubPlugin::GetHubPtr();

			if (!pHub)
			{
				SCHEMATYC_ENV_WARNING("CSchematycUqsComponent::SchematycFunction_StartQuery: UQS system is not available");
				return;
			}

			const Client::SQueryRequest request(m_upcomingQueryInfo.queryBlueprintID, m_upcomingQueryInfo.runtimeParams, m_upcomingQueryInfo.querierName.c_str(), functor(*this, &CSchematycUqsComponent::OnQueryResult));
			Shared::CUqsString errorMessage;

			const CQueryID queryID = pHub->GetQueryManager().StartQuery(request, errorMessage);

			if (queryID.IsValid())
			{
				m_runningQueries.insert(queryID);
				outQueryId.queryId = queryID;
			}
			else
			{
				// FIXME: should we maybe propagate this through the SQueryExceptionSignal ? (but then again, that signal needs a valid QueryID)
				SCHEMATYC_ENV_WARNING("CSchematycUqsComponent::SchematycFunction_StartQuery: unable to start query '%s', error: %s", m_upcomingQueryInfo.queryBlueprintID.GetQueryBlueprintName(), errorMessage.c_str());
			}

			m_upcomingQueryInfo.Clear();
		}

		void CSchematycUqsComponent::Update(const Schematyc::SUpdateContext& updateContext)
		{
			FlushFinishedQueries();
		}

		void CSchematycUqsComponent::CancelRunningQueriesAndClearFinishedOnes()
		{
			if (IHub* pHub = IHubPlugin::GetHubPtr())
			{
				IQueryManager& queryManager = pHub->GetQueryManager();

				for (const CQueryID& queryID : m_runningQueries)
				{
					queryManager.CancelQuery(queryID);
				}
			}

			m_runningQueries.clear();
			m_succeededQueries.clear();
			m_failedQueries.clear();
		}

		void CSchematycUqsComponent::FlushFinishedQueries()
		{
			// fire signals for all queries that finished without exception
			for (const auto& pair : m_succeededQueries)
			{
				const SQueryIdWrapper queryID(pair.first);
				const int32 resultCount = (int32)pair.second.pQueryResultSet->GetResultCount();
				const SQueryFinishedSignal signal(queryID, resultCount);
				OutputSignal(signal);
			}

			// fire signals for all queries that finished with an exception
			for (const auto& pair : m_failedQueries)
			{
				const SQueryIdWrapper queryID(pair.first);
				const Schematyc::CSharedString exceptionMessage(pair.second.c_str());
				const SQueryExceptionSignal signal(queryID, exceptionMessage);
				OutputSignal(signal);
			}

			m_succeededQueries.clear();
			m_failedQueries.clear();
		}

		void CSchematycUqsComponent::OnQueryResult(const SQueryResult& queryResult)
		{
			m_runningQueries.erase(queryResult.queryID);

			switch (queryResult.status)
			{
			case SQueryResult::Success:
				m_succeededQueries[queryResult.queryID].pQueryResultSet = std::move(queryResult.pResultSet);
				break;

			case SQueryResult::ExceptionOccurred:
				m_failedQueries[queryResult.queryID] = queryResult.error;
				break;

			case SQueryResult::CanceledByHubTearDown:
				// nothing (?)
				break;

			default:
				assert(0);
			}
		}

	}
}

#endif // UQS_SCHEMATYC_SUPPORT
