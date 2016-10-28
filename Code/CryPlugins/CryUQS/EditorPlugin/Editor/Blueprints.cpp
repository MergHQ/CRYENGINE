// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "Blueprints.h"
#include "DocSerializationContext.h"
#include "Settings.h"

#include <CrySerialization/CryStrings.h>
#include <CrySerialization/Enum.h>
#include <CrySerialization/STL.h>

#include "SerializationHelpers.h"

//#pragma optimize("", off)
//#pragma inline_depth(0)

//////////////////////////////////////////////////////////////////////////

template<typename TSetFunc>
bool SerializeStringWithSetter(Serialization::IArchive& archive, const char* szName, const char* szLabel, const char* const szOldValue, TSetFunc setFunc)
{
	stack_string str(szOldValue);
	const bool res = archive(str, szName, szLabel);
	if (res && archive.isInput() && str != szOldValue)
	{
		setFunc(str);
	}
	return res;
}

static bool IsStringEmpty(const char* szStr)
{
	return !szStr || !szStr[0];
}

template<typename TString>
static bool IsStringEmpty(const TString& str)
{
	return str.empty();
}

namespace uqseditor
{

// TODO pavloi 2015.12.09: we shouldn't expose undefined type to UI, but right now I'm going to use it for debug purposes.
SERIALIZATION_ENUM_BEGIN(EEvaluatorType, "EvaluatorType")
SERIALIZATION_ENUM(EEvaluatorType::Instant, "Instant", "Instant")
SERIALIZATION_ENUM(EEvaluatorType::Deferred, "Deferred", "Deferred")
SERIALIZATION_ENUM(EEvaluatorType::Undefined, "Undefined", "Undefined")
SERIALIZATION_ENUM_END()

//////////////////////////////////////////////////////////////////////////

class CSyntaxErrorCollector : public uqs::datasource::ISyntaxErrorCollector
{
public:
	explicit CSyntaxErrorCollector(const std::shared_ptr<CErrorCollector>& pErrorCollector)
		: m_pErrorCollector(pErrorCollector)
	{}

	// ISyntaxErrorCollector
	virtual void AddErrorMessage(const char* szFormat, ...) PRINTF_PARAMS(2, 3) override
	{
		va_list args;
		va_start(args, szFormat);
		string msg;
		msg.FormatV(szFormat, args);
		m_pErrorCollector->AddErrorMessage(std::move(msg));
		va_end(args);
	}
	// ~ISyntaxErrorCollector

private:
	std::shared_ptr<CErrorCollector> m_pErrorCollector;
};

//////////////////////////////////////////////////////////////////////////

static void DeleteSyntaxErrorCollector(uqs::datasource::ISyntaxErrorCollector* pSyntaxErrorCollectorToDelete)
{
	delete pSyntaxErrorCollectorToDelete;
}

static uqs::datasource::SyntaxErrorCollectorUniquePtr MakeNewSyntaxErrorCollectorUniquePtr(const std::shared_ptr<CErrorCollector>& pErrorCollector)
{
	return uqs::datasource::SyntaxErrorCollectorUniquePtr(new CSyntaxErrorCollector(pErrorCollector), uqs::datasource::CSyntaxErrorCollectorDeleter(&DeleteSyntaxErrorCollector));
}

//////////////////////////////////////////////////////////////////////////

class CEvaluatorFactoryHelper
{
public:
	CEvaluatorFactoryHelper()
		: m_pInstantFactory(nullptr)
		, m_pDeferredFactory(nullptr)
	{}

	CEvaluatorFactoryHelper(const CUqsDocSerializationContext& context, string name)
		: m_pInstantFactory(nullptr)
		, m_pDeferredFactory(nullptr)
	{
		m_pInstantFactory = context.GetInstantEvaluatorFactoryByName(name.c_str());
		if (!m_pInstantFactory)
		{
			m_pDeferredFactory = context.GetDeferredEvaluatorFactoryByName(name.c_str());
		}
	}

	const uqs::client::IInputParameterRegistry& GetInputParameterRegistry() const
	{
		assert(IsValid());
		if (m_pInstantFactory)
		{
			return m_pInstantFactory->GetInputParameterRegistry();
		}
		else
		{
			assert(m_pDeferredFactory);
			return m_pDeferredFactory->GetInputParameterRegistry();
		}
	}

	bool IsValid() const
	{
		assert(!(m_pInstantFactory && m_pDeferredFactory));
		return m_pInstantFactory || m_pDeferredFactory;
	}

	EEvaluatorType GetType() const
	{
		if (m_pInstantFactory)
		{
			return EEvaluatorType::Instant;
		}
		else if (m_pDeferredFactory)
		{
			return EEvaluatorType::Deferred;
		}
		else
		{
			return EEvaluatorType::Undefined;
		}
	}

private:
	uqs::client::IInstantEvaluatorFactory*  m_pInstantFactory;
	uqs::client::IDeferredEvaluatorFactory* m_pDeferredFactory;
};

//////////////////////////////////////////////////////////////////////////
// CErrorCollector
//////////////////////////////////////////////////////////////////////////

CErrorCollector::CErrorCollector()
	: m_messages()
	, m_pExternalCollector()
{}

CErrorCollector::CErrorCollector(CErrorCollector&& other)
	: m_messages(std::move(other.m_messages))
	, m_pExternalCollector(std::move(other.m_pExternalCollector))
{}

CErrorCollector& CErrorCollector::operator=(CErrorCollector&& other)
{
	if (this != &other)
	{
		m_messages = std::move(other.m_messages);
		m_pExternalCollector = std::move(other.m_pExternalCollector);
	}
	return *this;
}

void CErrorCollector::AddErrorMessage(string&& message)
{
	m_messages.push_back(message);
}

void CErrorCollector::Clear()
{
	m_messages.clear();
	m_pExternalCollector.reset();
}

void CErrorCollector::Serialize(Serialization::IArchive& archive, const SValidatorKey& validatorKey)
{
	if (archive.isValidation())
	{
		for (const string& message : m_messages)
		{
			archive.error(validatorKey.pHandle, validatorKey.typeId, "%s", message.c_str());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// CItemUniquePtr
//////////////////////////////////////////////////////////////////////////

CItemUniquePtr::CItemUniquePtr()
	: m_pItem(nullptr)
	, m_pItemFactory(nullptr)
{}

CItemUniquePtr::~CItemUniquePtr()
{
	if (m_pItem)
	{
		assert(m_pItemFactory);
		m_pItemFactory->DestroyItems(m_pItem);
		m_pItem = nullptr;
	}
}

CItemUniquePtr::CItemUniquePtr(CItemUniquePtr&& other)
	: m_pItem(other.m_pItem)
	, m_pItemFactory(other.m_pItemFactory)
{
	other.m_pItem = nullptr;
	other.m_pItemFactory = nullptr;
}

CItemUniquePtr& CItemUniquePtr::operator=(CItemUniquePtr&& other)
{
	m_pItem = other.m_pItem;
	m_pItemFactory = other.m_pItemFactory;
	other.m_pItem = nullptr;
	other.m_pItemFactory = nullptr;
	return *this;
}

CItemUniquePtr::CItemUniquePtr(uqs::client::IItemFactory* pItemFactory)
	: m_pItem(nullptr)
	, m_pItemFactory(pItemFactory)
{
	if (pItemFactory->CanBePersistantlySerialized())
	{
		m_pItem = pItemFactory->CreateItems(1, uqs::client::IItemFactory::EItemInitMode::UseUserProvidedFunction);
	}
}

bool CItemUniquePtr::IsSerializable() const
{
	return m_pItem && m_pItemFactory && m_pItemFactory->CanBePersistantlySerialized();
}

bool CItemUniquePtr::SetFromStringLiteral(const string& stringLiteral)
{
	if (m_pItem && m_pItemFactory)
	{
		return uqs::core::IHubPlugin::GetHub().GetItemSerializationSupport().DeserializeItemFromCStringLiteral(m_pItem, *m_pItemFactory, stringLiteral, nullptr);
	}
	return false;
}

void CItemUniquePtr::ConvertToStringLiteral(string& outString) const
{
	outString.clear();
	if (m_pItem && m_pItemFactory)
	{
		uqs::shared::CUqsString literalString;
		if (uqs::core::IHubPlugin::GetHub().GetItemSerializationSupport().SerializeItemToStringLiteral(m_pItem, *m_pItemFactory, literalString))
		{
			outString = literalString.c_str();
		}
	}
}

bool Serialize(Serialization::IArchive& archive, CItemUniquePtr& value, const char* szName, const char* szLabel)
{
	if (value.m_pItem && value.m_pItemFactory)
	{
		if (archive.isInput())
		{
			return value.m_pItemFactory->TryDeserializeItem(value.m_pItem, archive, szName, szLabel);
		}
		else
		{
			assert(archive.isOutput());
			return value.m_pItemFactory->TrySerializeItem(value.m_pItem, archive, szName, szLabel);
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
// CItemLiteral
//////////////////////////////////////////////////////////////////////////

CItemLiteral::CItemLiteral()
	: m_item()
	, m_typeName()
{}

CItemLiteral::CItemLiteral(const SItemTypeName& typeName, const CUqsDocSerializationContext& context)
	: m_item(context.GetItemFactoryByName(typeName))
	, m_typeName(typeName)
{}


bool CItemLiteral::SetFromStringLiteral(const string& stringLiteral)
{
	return m_item.SetFromStringLiteral(stringLiteral);
}

void CItemLiteral::ConvertToStringLiteral(string& outString) const
{
	m_item.ConvertToStringLiteral(outString);
}

bool Serialize(Serialization::IArchive& archive, CItemLiteral& value, const char* szName, const char* szLabel)
{
	return archive(value.m_item, szName, szLabel);
}


//////////////////////////////////////////////////////////////////////////
// CFunctionSerializationHelper
//////////////////////////////////////////////////////////////////////////

void CFunctionSerializationHelper::CFunctionList::Build(const SItemTypeName& typeName, const CUqsDocSerializationContext& context)
{
	const Serialization::StringList& namesList = context.GetFunctionNamesList(typeName);

	std::vector<SFunction> allFunctions;
	allFunctions.reserve(namesList.size());
	for (const auto& name : namesList)
	{
		if (uqs::client::IFunctionFactory* pFactory = context.GetFunctionFactoryByName(name.c_str()))
		{
			allFunctions.emplace_back();
			SFunction& func = allFunctions.back();

			func.internalName = name;
			func.pFactory = pFactory;
			func.leafFunctionKind = pFactory->GetLeafFunctionKind();
			func.returnType = context.GetItemTypeNameFromType(pFactory->GetReturnType());
		}
	}

	const bool bApplyTypeFilter = !typeName.Empty() && context.GetSettings().bFilterAvailableInputsByType;

	std::vector<SFunction> functions;
	functions.reserve(allFunctions.size());

	// iterated params
	if (CSelectedGeneratorContext* pSelectedGeneratorContext = context.GetSelectedGeneratorContext())
	{
		if (pSelectedGeneratorContext->GetGeneratorProcessed())
		{
			std::vector<SFunction> functionsForIteratedItem;
			std::copy_if(allFunctions.begin(), allFunctions.end(), std::back_inserter(functionsForIteratedItem),
			             [](const SFunction& func) { return func.leafFunctionKind == uqs::client::IFunctionFactory::ELeafFunctionKind::IteratedItem; });

			pSelectedGeneratorContext->BuildFunctionListForAvailableGenerators(typeName, context, functionsForIteratedItem, functions);

			for (SFunction& func : functions)
			{
				if (bApplyTypeFilter)
				{
					// TODO pavloi 2016.04.26: change, if we're going to support multiple generators
					func.prettyName = "ITER";
					// TODO pavloi 2016.04.26: is it possible to register several iterated item functions for same type?
					break;
				}
				else
				{
					func.prettyName.Format("ITER: %s", func.returnType.c_str());
				}
			}
		}
	}

	// Global Params
	if (CParametersListContext* pParams = context.GetParametersListContext())
	{
		std::vector<SFunction> globalParams;
		{
			std::vector<SFunction> functionsForGlobalParams;
			std::copy_if(allFunctions.begin(), allFunctions.end(), std::back_inserter(functionsForGlobalParams),
			             [](const SFunction& func) { return func.leafFunctionKind == uqs::client::IFunctionFactory::ELeafFunctionKind::GlobalParam; });

			pParams->BuildFunctionListForAvailableParameters(typeName, context, functionsForGlobalParams, globalParams);
		}

		for (SFunction& func : globalParams)
		{
			assert(func.leafFunctionKind == uqs::client::IFunctionFactory::ELeafFunctionKind::GlobalParam);
			assert(!func.param.empty());

			functions.emplace_back(std::move(func));
			SFunction& iteratedItemFunc = functions.back();

			iteratedItemFunc.prettyName.Format("PARAM: %s", iteratedItemFunc.param.c_str());
		}
		globalParams.clear();
	}

	// Literals
	{
		for (const SFunction& func : allFunctions)
		{
			if (func.leafFunctionKind == uqs::client::IFunctionFactory::ELeafFunctionKind::Literal)
			{
				functions.emplace_back(func);
				SFunction& iteratedItemFunc = functions.back();

				iteratedItemFunc.prettyName.Format("LITERAL: %s", iteratedItemFunc.returnType);

				if (bApplyTypeFilter)
				{
					// TODO pavloi 2016.04.26: is it possible to register several literal functions for same type?
					break;
				}
			}
		}
	}

	// Shuttled Items
	// TODO: the expectedShuttleType already gets properly serialized and such, but we also need a way for the user to specify the type somehow (preferably through a drop-down-list?)
	for (const SFunction& func : allFunctions)
	{
		if (func.leafFunctionKind == uqs::client::IFunctionFactory::ELeafFunctionKind::ShuttledItems)
		{
			functions.emplace_back(func);
			SFunction& shuttledItemsFunc = functions.back();

			shuttledItemsFunc.prettyName.Format("SHUTTLE: %s", shuttledItemsFunc.returnType);

			if (bApplyTypeFilter)
			{
				// TODO christianw 2016.09.05: is it possible to register several shuttled-items functions for same type?
				break;
			}
		}
	}

	// All other functions
	{
		for (const SFunction& func : allFunctions)
		{
			auto alreadyListedFuncIter = std::find_if(functions.begin(), functions.end(), [&func](const SFunction& alreadyListedFunc) { return alreadyListedFunc.pFactory == func.pFactory; });
			if (alreadyListedFuncIter == functions.end())
			{
				functions.emplace_back(func);
				SFunction& iteratedItemFunc = functions.back();

				iteratedItemFunc.prettyName = iteratedItemFunc.internalName;
			}
		}
	}

	// Build string list
	{
		m_functionsStringList.clear();
		m_functionsStringList.reserve(functions.size() + 1);
		m_functionsStringList.push_back(string());
		for (const SFunction& func : functions)
		{
			m_functionsStringList.push_back(func.prettyName);
		}
	}

	m_functions = std::move(functions);
}

int CFunctionSerializationHelper::CFunctionList::SerializeName(
  Serialization::IArchive& archive,
  const char* szName,
  const char* szLabel,
  const CUqsDocSerializationContext& context,
  const SValidatorKey& validatorKey,
  const int oldFunctionIdx)
{
	const bool bIsNpos = (oldFunctionIdx == Serialization::StringList::npos);

	Serialization::StringListValue strValue(m_functionsStringList, !bIsNpos ? (oldFunctionIdx + 1) : 0, validatorKey.pHandle, validatorKey.typeId);
	const int oldStrValueIndex = strValue.index();
	const bool bRes = archive(strValue, szName, szLabel);

	const int newStrValueIndex = strValue.index();
	int resultFunctionIndex = oldFunctionIdx;

	const bool bChanged = oldStrValueIndex != newStrValueIndex;
	if (bChanged)
	{
		if (newStrValueIndex == Serialization::StringList::npos || newStrValueIndex == 0)
		{
			resultFunctionIndex = CFunctionSerializationHelper::npos;
		}
		else
		{
			resultFunctionIndex = newStrValueIndex - 1;
		}
	}

	return resultFunctionIndex;
}

const CFunctionSerializationHelper::SFunction& CFunctionSerializationHelper::CFunctionList::GetByIdx(const int idx) const
{
	if (idx < 0 || idx > m_functions.size())
	{
		CryFatalError("Wrong index");
	}

	return m_functions[idx];
}

int CFunctionSerializationHelper::CFunctionList::FindByInternalNameAndParam(const string& internalName, const string& param) const
{
	for (int i = 0; i < m_functions.size(); ++i)
	{
		if (m_functions[i].internalName == internalName)
		{
			if (m_functions[i].leafFunctionKind == uqs::client::IFunctionFactory::ELeafFunctionKind::GlobalParam)
			{
				if (m_functions[i].param == param)
				{
					return i;
				}

				// TODO pavloi 2016.04.26: remember, that the function was a GlobalParam leaf. If no params are found, stop search (list is sorted) and use
				// this info to show good warning message.
			}
			else
			{
				return i;
			}
		}
	}
	return CFunctionSerializationHelper::npos;
}

void CFunctionSerializationHelper::CFunctionList::Clear()
{
	m_functions.clear();
}

CFunctionSerializationHelper::CFunctionSerializationHelper()
	: m_funcInternalName()
	, m_funcParam()
	, m_itemLiteral()
	, m_typeName()
	, m_functionsList()
	, m_selectedFunctionIdx(npos)
{}

CFunctionSerializationHelper::CFunctionSerializationHelper(const char* szFunctionName, const char* szParamOrReturnValue)
	: m_funcInternalName(szFunctionName)
	, m_funcParam(szParamOrReturnValue)
	, m_itemLiteral()
	, m_typeName()
	, m_functionsList()
	, m_selectedFunctionIdx(npos)
{}

CFunctionSerializationHelper::CFunctionSerializationHelper(const uqs::client::IFunctionFactory& functionFactory, const CUqsDocSerializationContext& context)
	: m_funcInternalName(functionFactory.GetName())
	, m_funcParam()
	, m_itemLiteral()
	, m_typeName(context.GetItemTypeNameFromType(functionFactory.GetReturnType()))
	, m_functionsList()
	, m_selectedFunctionIdx(npos)
{}

void CFunctionSerializationHelper::RebuildList(const CUqsDocSerializationContext& context)
{
	m_functionsList.Clear();
	if (context.GetSettings().bUseSelectionHelpers && !m_typeName.Empty())
	{
		m_functionsList.Build(m_typeName, context);
	}
}

void CFunctionSerializationHelper::UpdateSelectedFunctionIndex()
{
	m_selectedFunctionIdx = m_functionsList.FindByInternalNameAndParam(m_funcInternalName, m_funcParam);
}

void CFunctionSerializationHelper::Prepare(const CUqsDocSerializationContext& context)
{
	RebuildList(context);
	UpdateSelectedFunctionIndex();
}

void CFunctionSerializationHelper::Reset(const SItemTypeName& typeName, const CUqsDocSerializationContext& context)
{
	bool bShouldResetList = context.ShouldForceSelectionHelpersEvaluation();

	if (m_typeName != typeName)
	{
		m_typeName = typeName;
		m_itemLiteral = CItemLiteral();

		bShouldResetList = true;
	}

	if (!bShouldResetList && context.GetParametersListContext())
	{
		bShouldResetList = context.GetParametersListContext()->GetParamsChanged();
	}

	if (bShouldResetList)
	{
		Prepare(context);

		if (m_selectedFunctionIdx != npos)
		{
			const SFunction& function = m_functionsList.GetByIdx(m_selectedFunctionIdx);
			const bool bIsLiteral = (function.leafFunctionKind == uqs::client::IFunctionFactory::ELeafFunctionKind::Literal);
			
			if (bIsLiteral)
			{
				m_itemLiteral = CItemLiteral(typeName, context);

				if (m_itemLiteral.IsSerializable() && !IsStringEmpty(m_funcParam))
				{
					// TODO pavloi 2016.09.01: this should happen when we read everything from textual blueprint, 
					// but the code and initialization order I wrote is such a dense spaghetti, that it doesn't seem easy anymore.
					if (m_itemLiteral.SetFromStringLiteral(m_funcParam))
					{
						m_funcParam.clear();
					}
				}
			}
		}
	}
}

bool CFunctionSerializationHelper::SerializeFunctionName(
  Serialization::IArchive& archive,
  const char* szName,
  const char* szLabel,
  const CUqsDocSerializationContext& context)
{
	bool bNameChanged = false;

	if (context.GetSettings().bUseSelectionHelpers)
	{
		const int oldFunctionIdx = m_selectedFunctionIdx;
		const SValidatorKey validatorKey = SValidatorKey::FromObject(*this);
		const int functionIdx = m_functionsList.SerializeName(archive, szName, szLabel, context, validatorKey, oldFunctionIdx);

		if (functionIdx != oldFunctionIdx)
		{
			bNameChanged = true;
			m_selectedFunctionIdx = functionIdx;
			if (functionIdx != npos)
			{
				const SFunction& function = m_functionsList.GetByIdx(functionIdx);
				m_funcInternalName = function.internalName;
				
				switch (function.leafFunctionKind)
				{
				case uqs::client::IFunctionFactory::ELeafFunctionKind::GlobalParam:
					m_funcParam = function.param;
					m_itemLiteral = CItemLiteral();
					break;

				case uqs::client::IFunctionFactory::ELeafFunctionKind::Literal:
					if (!m_itemLiteral.IsExist())
					{
						m_itemLiteral = CItemLiteral(function.returnType, context);
					}
					break;

				default:
					m_funcParam.clear();
					m_itemLiteral = CItemLiteral();
					break;
				}
			}
			else
			{
				m_funcInternalName.clear();
				
				m_funcParam.clear();
				m_itemLiteral = CItemLiteral();
			}
		}

		if (m_selectedFunctionIdx == npos && !m_funcInternalName.empty())
		{
			// TODO pavloi 2016.04.26: there is no way to pass validator key handle and type to the warning() like to error()

			if (m_funcParam.empty())
			{
				archive.warning(*this, "Function '%s' is not a valid input.", m_funcInternalName.c_str());
			}
			else
			{
				archive.warning(*this, "Function '%s' with param '%s' is not a valid input.", m_funcInternalName.c_str(), m_funcParam.c_str());
			}
		}
	}
	else
	{
		const char* szOldName = m_funcInternalName.c_str();
		const auto setName = [this, &bNameChanged](const char* szStr)
		{
			m_funcInternalName = szStr;
			bNameChanged = true;
		};

		SerializeStringWithSetter(archive, szName, szLabel, szOldName, setName);
	}

	return bNameChanged;
}

void CFunctionSerializationHelper::SerializeFunctionParam(Serialization::IArchive& archive, const CUqsDocSerializationContext& context)
{
	if (context.GetSettings().bUseSelectionHelpers)
	{
		if (m_selectedFunctionIdx != npos)
		{
			const SFunction& function = m_functionsList.GetByIdx(m_selectedFunctionIdx);

			switch (function.leafFunctionKind)
			{
			case uqs::client::IFunctionFactory::ELeafFunctionKind::Literal:
				if (m_itemLiteral.IsExist())
				{
					archive(m_itemLiteral, "returnValue", "^");
				}
				else
				{
					archive(m_funcParam, "returnValue", "^");
				}
				break;

			default:
				// do nothing
				break;
			}
		}
	}
	else
	{
		stack_string tmp("Not supported");
		archive(tmp, "returnValue", "!Return Value");
	}
}

const string& CFunctionSerializationHelper::GetFunctionInternalName() const
{
	return m_funcInternalName;
}

const string& CFunctionSerializationHelper::GetFunctionParamOrReturnValue() const
{
	if (m_itemLiteral.IsExist())
	{
		m_itemLiteral.ConvertToStringLiteral(m_itemCachedLiteral);
		return m_itemCachedLiteral;
	}
	else
	{
		return m_funcParam;
	}
}

uqs::client::IFunctionFactory* CFunctionSerializationHelper::GetFunctionFactory() const
{
	if (const SFunction* pFunction = GetSelectedFunction())
	{
		return pFunction->pFactory;
	}
	return nullptr;
}

const CFunctionSerializationHelper::SFunction* CFunctionSerializationHelper::GetSelectedFunction() const
{
	if (m_selectedFunctionIdx == npos)
	{
		return nullptr;
	}

	return &m_functionsList.GetByIdx(m_selectedFunctionIdx);
}

//////////////////////////////////////////////////////////////////////////
// CInputBlueprint
//////////////////////////////////////////////////////////////////////////

CInputBlueprint::CInputBlueprint()
	: m_paramName()
	, m_addReturnValueToDebugRenderWorldUponExecution("false")
	, m_functionHelper()
	, m_children()
	, m_pErrorCollector(new CErrorCollector)
{}

CInputBlueprint::CInputBlueprint(const char* szParamName, const char* szFuncName, const char* szFuncReturnValueLiteral, const char* szAddReturnValueToDebugRenderWorldUponExecution)
	: m_paramName(szParamName)
	, m_addReturnValueToDebugRenderWorldUponExecution(szAddReturnValueToDebugRenderWorldUponExecution)
	, m_functionHelper(szFuncName, szFuncReturnValueLiteral)
	, m_children()
	, m_pErrorCollector(new CErrorCollector)
{}

CInputBlueprint::CInputBlueprint(const char* szParamName)
	: m_paramName(szParamName)
	, m_addReturnValueToDebugRenderWorldUponExecution("false")
	, m_functionHelper()
	, m_children()
	, m_pErrorCollector(new CErrorCollector)
{}

CInputBlueprint::CInputBlueprint(const uqs::client::IFunctionFactory& functionFactory, const CUqsDocSerializationContext& context)
	: m_paramName()
	, m_addReturnValueToDebugRenderWorldUponExecution("false")
	, m_functionHelper(functionFactory, context)
	, m_children()
	, m_pErrorCollector(new CErrorCollector)
{
	ResetChildrenFromFactory(functionFactory, context);
}

CInputBlueprint::CInputBlueprint(CInputBlueprint&& other)
	: m_paramName(std::move(other.m_paramName))
	, m_addReturnValueToDebugRenderWorldUponExecution(std::move(other.m_addReturnValueToDebugRenderWorldUponExecution))
	, m_functionHelper(std::move(other.m_functionHelper))
	, m_children(std::move(other.m_children))
	, m_pErrorCollector(std::move(other.m_pErrorCollector))
{}

CInputBlueprint& CInputBlueprint::operator=(CInputBlueprint&& other)
{
	if (this != &other)
	{
		m_paramName = std::move(other.m_paramName);
		m_addReturnValueToDebugRenderWorldUponExecution = std::move(other.m_addReturnValueToDebugRenderWorldUponExecution);
		m_functionHelper = std::move(other.m_functionHelper);
		m_children = std::move(other.m_children);
		m_pErrorCollector = std::move(other.m_pErrorCollector);
	}
	return *this;
}

void CInputBlueprint::ResetChildrenFromFactory(const uqs::client::IGeneratorFactory& generatorFactory, const CUqsDocSerializationContext& context)
{
	m_children.clear();
	SetChildrenFromFactoryInputRegistry(generatorFactory, context);
}

void CInputBlueprint::ResetChildrenFromFactory(const uqs::client::IFunctionFactory& functionFactory, const CUqsDocSerializationContext& context)
{
	m_children.clear();
	SetChildrenFromFactoryInputRegistry(functionFactory, context);
}

void CInputBlueprint::ResetChildrenFromFactory(const CEvaluatorFactoryHelper& evaluatorFactory, const CUqsDocSerializationContext& context)
{
	m_children.clear();
	SetChildrenFromFactoryInputRegistry(evaluatorFactory, context);
}

template<typename TFactory>
void CInputBlueprint::SetChildrenFromFactoryInputRegistry(const TFactory& factory, const CUqsDocSerializationContext& context)
{
	const uqs::client::IInputParameterRegistry& inputRegistry = factory.GetInputParameterRegistry();
	const size_t paramCount = inputRegistry.GetParameterCount();
	for (size_t i = 0; i < paramCount; ++i)
	{
		const uqs::client::IInputParameterRegistry::SParameterInfo paramInfo = inputRegistry.GetParameter(i);
		const CInputBlueprint* pParam = FindChildByParamName(paramInfo.name);
		if (!pParam)
		{
			m_children.emplace_back(paramInfo.name);
			m_children.back().SetAdditionalParamInfo(paramInfo, context);
		}
	}
}

void CInputBlueprint::PrepareHelpers(const uqs::client::IGeneratorFactory* pGeneratorFactory, const CUqsDocSerializationContext& context)
{
	if (pGeneratorFactory)
	{
		DeriveChildrenInfoFromFactoryInputRegistry(*pGeneratorFactory, context);
	}

	PrepareHelpersRoot(context);
}

void CInputBlueprint::PrepareHelpers(const uqs::client::IFunctionFactory* pFunctionFactory, const CUqsDocSerializationContext& context)
{
	if (pFunctionFactory)
	{
		DeriveChildrenInfoFromFactoryInputRegistry(*pFunctionFactory, context);
	}

	PrepareHelpersRoot(context);
}

void CInputBlueprint::PrepareHelpers(const CEvaluatorFactoryHelper* pEvaluatorFactory, const CUqsDocSerializationContext& context)
{
	if (pEvaluatorFactory)
	{
		DeriveChildrenInfoFromFactoryInputRegistry(*pEvaluatorFactory, context);
	}

	PrepareHelpersRoot(context);
}

void CInputBlueprint::PrepareHelpersRoot(const CUqsDocSerializationContext& context)
{
	for (CInputBlueprint& child : m_children)
	{
		child.PrepareHelpersChild(context);
	}
}

void CInputBlueprint::PrepareHelpersChild(const CUqsDocSerializationContext& context)
{
	uqs::client::IFunctionFactory* pFunctionFactory = m_functionHelper.GetFunctionFactory();
	PrepareHelpers(pFunctionFactory, context);
}

template<typename TFactory>
void CInputBlueprint::DeriveChildrenInfoFromFactoryInputRegistry(const TFactory& factory, const CUqsDocSerializationContext& context)
{
	const uqs::client::IInputParameterRegistry& inputRegistry = factory.GetInputParameterRegistry();

	const size_t paramCount = inputRegistry.GetParameterCount();
	std::map<string, size_t> nameToParameterInfoIdx;
	for (size_t i = 0; i < paramCount; ++i)
	{
		const uqs::client::IInputParameterRegistry::SParameterInfo paramInfo = inputRegistry.GetParameter(i);
		nameToParameterInfoIdx[string(paramInfo.name)] = i;
	}

	for (CInputBlueprint& child : m_children)
	{
		auto iter = nameToParameterInfoIdx.find(child.m_paramName);
		if (iter != nameToParameterInfoIdx.end())
		{
			child.SetAdditionalParamInfo(inputRegistry.GetParameter(iter->second), context);
		}
	}
}

void CInputBlueprint::SetAdditionalParamInfo(const uqs::client::IInputParameterRegistry::SParameterInfo& paramInfo, const CUqsDocSerializationContext& context)
{
	m_functionHelper.Reset(context.GetItemTypeNameFromType(paramInfo.type), context);
}

void CInputBlueprint::Serialize(Serialization::IArchive& archive)
{
	assert(m_pErrorCollector);

	CUqsDocSerializationContext* pContext = archive.context<CUqsDocSerializationContext>();
	if (!pContext)
	{
		assert(false);
		return;
	}

	m_pErrorCollector->Serialize(archive, *this);

	const bool bUseSelectionHelpers = pContext->GetSettings().bUseSelectionHelpers;
	const bool bShowInputParamTypes = pContext->GetSettings().bShowInputParamTypes;

	if (bUseSelectionHelpers)
	{
		// Prepare param (and type) name and use it as label for function
		if (bShowInputParamTypes)
		{
			m_paramLabel.Format("^%s (%s)", m_paramName.c_str(), m_functionHelper.GetExpectedType().c_str());
		}
		else
		{
			m_paramLabel.Format("^%s", m_paramName.c_str());
		}

		SerializeFunction(archive, *pContext, m_paramLabel.c_str());
	}
	else
	{
		archive(m_paramName, "paramName", "^Param");
		SerializeFunction(archive, *pContext, "^Func");
	}
}

void CInputBlueprint::SerializeRoot(Serialization::IArchive& archive, const char* szName, const char* szLabel, const CUqsDocSerializationContext& context, const SValidatorKey& validatorKey)
{
	assert(m_pErrorCollector);
	m_pErrorCollector->Serialize(archive, validatorKey);
	SerializeChildren(archive, szName, szLabel, context);
}

void CInputBlueprint::SerializeFunction(Serialization::IArchive& archive, const CUqsDocSerializationContext& context, const char* szParamLabel)
{
	uqs::client::IFunctionFactory* pFactory = nullptr;

	const bool bNameChanged = m_functionHelper.SerializeFunctionName(archive, "funcName", szParamLabel, context);

	bool bIsLeafUndecided = true;
	uqs::client::IFunctionFactory::ELeafFunctionKind leafFunctionKind =
	  uqs::client::IFunctionFactory::ELeafFunctionKind::None;

	if (context.GetSettings().bUseSelectionHelpers)
	{
		pFactory = m_functionHelper.GetFunctionFactory();

		if (pFactory)
		{
			bIsLeafUndecided = false;
			leafFunctionKind = pFactory->GetLeafFunctionKind();
		}
		else
		{
			return;
		}
	}

	if (pFactory)
	{
		if (bNameChanged)
		{
			ResetChildrenFromFactory(*pFactory, context);
		}
	}

	if (bNameChanged)
	{
		// Name was changed - return right now, so PropertyTree will not try
		// to insert children of previous function into the new one.
		// TODO pavloi 2016.04.25: #FIX_COPY
		return;
	}

	if (bIsLeafUndecided)
	{
		m_functionHelper.SerializeFunctionParam(archive, context);

		const bool bIsLeafFunc = !IsStringEmpty(m_functionHelper.GetFunctionParamOrReturnValue());
		if (!bIsLeafFunc)
		{
			SerializeChildren(archive, "inputs", "Inputs", context);
		}
	}
	else
	{
		switch (leafFunctionKind)
		{
		case uqs::client::IFunctionFactory::ELeafFunctionKind::None:
			SerializeChildren(archive, "inputs", "Inputs", context);
			break;

		case uqs::client::IFunctionFactory::ELeafFunctionKind::IteratedItem:
			// do nothing
			break;

		case uqs::client::IFunctionFactory::ELeafFunctionKind::GlobalParam:
			// do nothing
			break;

		case uqs::client::IFunctionFactory::ELeafFunctionKind::Literal:
			m_functionHelper.SerializeFunctionParam(archive, context);
			break;
		}
	}
}

void CInputBlueprint::SerializeChildren(Serialization::IArchive& archive, const char* szName, const char* szLabel, const CUqsDocSerializationContext& context)
{
	if (context.GetSettings().bUseSelectionHelpers)
	{
		if (m_children.size() == 1)
		{
			// NOTE pavloi 2016.04.26: special case for functions with one child - it will be serialized not as container to
			// remove some visual noise from UI
			archive(m_children[0], szName, "<0.");   // label looks like a container's element
		}
		else
		{
			archive(FixedContainerSTL(m_children), szName, szLabel);
		}
	}
	else
	{
		archive(m_children, szName, szLabel);
	}
}

void CInputBlueprint::ClearErrors()
{
	assert(m_pErrorCollector);
	m_pErrorCollector->Clear();

	for (CInputBlueprint& input : m_children)
	{
		input.ClearErrors();
	}
}

//////////////////////////////////////////////////////////////////////////

const char* CInputBlueprint::GetParamName() const
{
	return m_paramName.c_str();
}

const char* CInputBlueprint::GetFuncName() const
{
	return m_functionHelper.GetFunctionInternalName().c_str();
}

const char* CInputBlueprint::GetFuncReturnValueLiteral() const
{
	return m_functionHelper.GetFunctionParamOrReturnValue().c_str();
}

const char* CInputBlueprint::GetAddReturnValueToDebugRenderWorldUponExecution() const
{
	return m_addReturnValueToDebugRenderWorldUponExecution.c_str();
}

CInputBlueprint& CInputBlueprint::AddChild(const char* szParamName, const char* szFuncName, const char* szFuncReturnValueLiteral, const char* szAddReturnValueToDebugRenderWorldUponExecution)
{
	m_children.emplace_back(szParamName, szFuncName, szFuncReturnValueLiteral, szAddReturnValueToDebugRenderWorldUponExecution);
	return m_children.back();
}

size_t CInputBlueprint::GetChildCount() const
{
	return m_children.size();
}

const CInputBlueprint& CInputBlueprint::GetChild(size_t index) const
{
	return m_children[index];
}

const CInputBlueprint* CInputBlueprint::FindChildByParamName(const char* paramName) const
{
	auto iter = std::find_if(m_children.begin(), m_children.end(),
	                         [paramName](const CInputBlueprint& bp) { return bp.m_paramName == paramName; });
	return iter != m_children.end() ? &*iter : nullptr;
}

//////////////////////////////////////////////////////////////////////////
// CConstParamBlueprint
//////////////////////////////////////////////////////////////////////////

CConstParamBlueprint::SConstParam::SConstParam()
	: pErrorCollector(new CErrorCollector)
{}

CConstParamBlueprint::SConstParam::SConstParam(const char* szName, const char* szType, CItemLiteral&& value)
	: name(szName)
	, type(szType)
	, value(std::move(value))
	, pErrorCollector(new CErrorCollector)
{}

CConstParamBlueprint::SConstParam::SConstParam(SConstParam&& other)
	: name(std::move(other.name))
	, type(std::move(other.type))
	, value(std::move(other.value))
	, pErrorCollector(std::move(other.pErrorCollector))
{}

CConstParamBlueprint::SConstParam& CConstParamBlueprint::SConstParam::operator=(CConstParamBlueprint::SConstParam&& other)
{
	if (this != &other)
	{
		name = std::move(other.name);
		type = std::move(other.type);
		value = std::move(other.value);
		pErrorCollector = std::move(other.pErrorCollector);
	}
	return *this;
}

void CConstParamBlueprint::SConstParam::Serialize(Serialization::IArchive& archive)
{
	if (CUqsDocSerializationContext* pContext = archive.context<CUqsDocSerializationContext>())
	{
		if (archive.isInput())
		{
			string oldName = name;
			SItemTypeName oldType = type;

			SerializeImpl(archive, *pContext);

			if (CParametersListContext* pParamsContext = pContext->GetParametersListContext())
			{
				if ((name != oldName) || (type != oldType))
				{
					pParamsContext->SetParamsChanged(true);
				}
			}
		}
		else
		{
			SerializeImpl(archive, *pContext);
		}
	}
	else
	{
		assert(false); // not supported
	}
}

void CConstParamBlueprint::SConstParam::SerializeImpl(Serialization::IArchive& archive, CUqsDocSerializationContext& context)
{
	pErrorCollector->Serialize(archive, *this);

	archive(name, "name", "^Name");
	archive(type, "type", "^Type");

	if (archive.isInput())
	{
		if (type != value.GetTypeName())
		{
			value = CItemLiteral(type, context);
		}
	}

	archive(value, "value", "^Value");
}

void CConstParamBlueprint::SConstParam::ClearErrors()
{
	assert(pErrorCollector);
	pErrorCollector->Clear();
}

bool CConstParamBlueprint::SConstParam::IsValid() const
{
	return !name.empty() && !type.Empty();
}

void CConstParamBlueprint::AddParameter(const char* szName, const char* szType, CItemLiteral&& value)
{
	m_params.emplace_back(szName, szType, std::move(value));
}

size_t CConstParamBlueprint::GetParameterCount() const
{
	return m_params.size();
}

void CConstParamBlueprint::GetParameterInfo(size_t index, const char*& szName, const char*& szType, string& value, std::shared_ptr<CErrorCollector>& pErrorCollector) const
{
	assert(index < m_params.size());

	const SConstParam& param = m_params[index];
	szName = param.name.c_str();
	szType = param.type.c_str();
	param.value.ConvertToStringLiteral(*&value);
	pErrorCollector = param.pErrorCollector;
}

void CConstParamBlueprint::Serialize(Serialization::IArchive& archive)
{
	const size_t oldSize = m_params.size();
	archive(m_params, "params", "^");

	if (m_params.size() != oldSize)
	{
		if (CUqsDocSerializationContext* pContext = archive.context<CUqsDocSerializationContext>())
		{
			if (CParametersListContext* pParamsContext = pContext->GetParametersListContext())
			{
				pParamsContext->SetParamsChanged(true);
			}
		}
	}
}

void CConstParamBlueprint::PrepareHelpers(CUqsDocSerializationContext& context)
{
	// nothing
}

void CConstParamBlueprint::ClearErrors()
{
	for (SConstParam& param : m_params)
	{
		param.ClearErrors();
	}
}

//////////////////////////////////////////////////////////////////////////
// CRuntimeParamBlueprint
//////////////////////////////////////////////////////////////////////////

CRuntimeParamBlueprint::SRuntimeParam::SRuntimeParam()
	: pErrorCollector(new CErrorCollector)
{}

CRuntimeParamBlueprint::SRuntimeParam::SRuntimeParam(const char* szName, const char* szType)
	: name(szName)
	, type(szType)
	, pErrorCollector(new CErrorCollector)
{}

CRuntimeParamBlueprint::SRuntimeParam::SRuntimeParam(CRuntimeParamBlueprint::SRuntimeParam&& other)
	: name(std::move(other.name))
	, type(std::move(other.type))
	, pErrorCollector(std::move(other.pErrorCollector))
{}

CRuntimeParamBlueprint::SRuntimeParam& CRuntimeParamBlueprint::SRuntimeParam::operator=(CRuntimeParamBlueprint::SRuntimeParam&& other)
{
	if (this != &other)
	{
		name = std::move(other.name);
		type = std::move(other.type);
		pErrorCollector = std::move(other.pErrorCollector);
	}
	return *this;
}

void CRuntimeParamBlueprint::SRuntimeParam::Serialize(Serialization::IArchive& archive)
{
	if (archive.isInput())
	{
		string oldName = name;
		SItemTypeName oldType = type;

		SerializeImpl(archive);

		if (CUqsDocSerializationContext* pContext = archive.context<CUqsDocSerializationContext>())
		{
			if (CParametersListContext* pParamsContext = pContext->GetParametersListContext())
			{
				if ((name != oldName) || (type != oldType))
				{
					pParamsContext->SetParamsChanged(true);
				}
			}
		}
	}
	else
	{
		SerializeImpl(archive);
	}
}

void CRuntimeParamBlueprint::SRuntimeParam::SerializeImpl(Serialization::IArchive& archive)
{
	pErrorCollector->Serialize(archive, *this);

	archive(name, "name", "^Name");
	archive(type, "type", "^Type");
}

void CRuntimeParamBlueprint::SRuntimeParam::ClearErrors()
{
	assert(pErrorCollector);
	pErrorCollector->Clear();
}

bool CRuntimeParamBlueprint::SRuntimeParam::IsValid() const
{
	return !name.empty() && !type.Empty();
}

void CRuntimeParamBlueprint::AddParameter(const char* szName, const char* szType)
{
	m_params.emplace_back(szName, szType);
}

size_t CRuntimeParamBlueprint::GetParameterCount() const
{
	return m_params.size();
}

void CRuntimeParamBlueprint::GetParameterInfo(size_t index, const char*& szName, const char*& szType, std::shared_ptr<CErrorCollector>& pErrorCollector) const
{
	assert(index < m_params.size());

	const SRuntimeParam& param = m_params[index];
	szName = param.name.c_str();
	szType = param.type.c_str();
	pErrorCollector = param.pErrorCollector;
}

void CRuntimeParamBlueprint::Serialize(Serialization::IArchive& archive)
{
	const size_t oldSize = m_params.size();
	archive(m_params, "params", "^");

	if (m_params.size() != oldSize)
	{
		if (CUqsDocSerializationContext* pContext = archive.context<CUqsDocSerializationContext>())
		{
			if (CParametersListContext* pParamsContext = pContext->GetParametersListContext())
			{
				pParamsContext->SetParamsChanged(true);
			}
		}
	}
}

void CRuntimeParamBlueprint::PrepareHelpers(CUqsDocSerializationContext& context)
{
	// nothing
}

void CRuntimeParamBlueprint::ClearErrors()
{
	for (SRuntimeParam& param : m_params)
	{
		param.ClearErrors();
	}
}

//////////////////////////////////////////////////////////////////////////
// CGeneratorBlueprint
//////////////////////////////////////////////////////////////////////////

CGeneratorBlueprint::CGeneratorBlueprint()
	: m_pErrorCollector(new CErrorCollector)
{}

CGeneratorBlueprint::CGeneratorBlueprint(const uqs::client::IGeneratorFactory& factory, const CUqsDocSerializationContext& context)
	: m_name(factory.GetName())
	, m_pErrorCollector(new CErrorCollector)
{
	m_inputs.ResetChildrenFromFactory(factory, context);
}

void CGeneratorBlueprint::SetGeneratorName(const char* szGeneratorName)
{
	m_name = szGeneratorName;
}

CInputBlueprint& CGeneratorBlueprint::GetInputRoot()
{
	return m_inputs;
}

const CInputBlueprint& CGeneratorBlueprint::GetInputRoot() const
{
	return m_inputs;
}

const char* CGeneratorBlueprint::GetGeneratorName() const
{
	return m_name.c_str();
}

void CGeneratorBlueprint::Serialize(Serialization::IArchive& archive)
{
	if (CUqsDocSerializationContext* pContext = archive.context<CUqsDocSerializationContext>())
	{
		assert(m_pErrorCollector);

		const bool bNameChanged = SerializeName(archive, "name", "^", *pContext);

		m_pErrorCollector->Serialize(archive, *this);

		if (!IsStringEmpty(m_name))
		{
			SerializeInputs(archive, "inputs", "Inputs", bNameChanged, *pContext);
		}

		if (bNameChanged)
		{
			if (pContext->GetSelectedGeneratorContext())
			{
				pContext->GetSelectedGeneratorContext()->SetGeneratorChanged(true);
			}
		}
	}
}

bool CGeneratorBlueprint::SerializeName(Serialization::IArchive& archive, const char* szName, const char* szLabel, const CUqsDocSerializationContext& context)
{
	bool bNameChanged = false;

	const char* szOldGeneratorName = m_name.c_str();
	auto setName = [this, &bNameChanged](const char* szNewValue)
	{
		m_name = szNewValue;
		bNameChanged = true;
	};

	if (context.GetSettings().bUseSelectionHelpers)
	{
		const Serialization::StringList& namesList = context.GetGeneratorNamesList();

		SerializeWithStringList(archive, szName, szLabel,
		                        namesList, szOldGeneratorName, setName);
	}
	else
	{
		SerializeStringWithSetter(archive, szName, szLabel, szOldGeneratorName, setName);
	}
	return bNameChanged;
}

void CGeneratorBlueprint::SerializeInputs(Serialization::IArchive& archive, const char* szName, const char* szLabel, const bool bNameChanged, const CUqsDocSerializationContext& context)
{
	uqs::client::IGeneratorFactory* pFactory = nullptr;

	if (context.GetSettings().bUseSelectionHelpers)
	{
		pFactory = context.GetGeneratorFactoryByName(m_name.c_str());
	}

	if (pFactory)
	{
		if (bNameChanged)
		{
			m_inputs.ResetChildrenFromFactory(*pFactory, context);
		}
	}

	if (bNameChanged)
	{
		// Name was changed - return right now, so PropertyTree will not try
		// to insert children of previous generator into the new one.
		return;
	}

	m_inputs.SerializeRoot(archive, szName, szLabel, context, SValidatorKey::FromObject(*this));
}

void CGeneratorBlueprint::PrepareHelpers(CUqsDocSerializationContext& context)
{
	uqs::client::IGeneratorFactory* pFactory = nullptr;

	if (context.GetSettings().bUseSelectionHelpers)
	{
		pFactory = context.GetGeneratorFactoryByName(m_name.c_str());
	}

	m_inputs.PrepareHelpers(pFactory, context);
}

void CGeneratorBlueprint::ClearErrors()
{
	assert(m_pErrorCollector);
	m_pErrorCollector->Clear();
	m_inputs.ClearErrors();
}

//////////////////////////////////////////////////////////////////////////
// CInstantEvaluatorBlueprint
//////////////////////////////////////////////////////////////////////////

void CInstantEvaluatorBlueprint::SetEvaluatorName(const char* szEvaluatorName)
{
	return Owner().SetEvaluatorName(szEvaluatorName);
}

void CInstantEvaluatorBlueprint::SetWeight(const char* szWeight)
{
	Owner().SetWeight(szWeight);
}

const char* CInstantEvaluatorBlueprint::GetWeight() const
{
	return Owner().GetWeight();
}

CInputBlueprint& CInstantEvaluatorBlueprint::GetInputRoot()
{
	return Owner().GetInputRoot();
}

const CInputBlueprint& CInstantEvaluatorBlueprint::GetInputRoot() const
{
	return Owner().GetInputRoot();
}

const char* CInstantEvaluatorBlueprint::GetEvaluatorName() const
{
	return Owner().GetEvaluatorName();
}

//////////////////////////////////////////////////////////////////////////
// CDeferredEvaluatorBlueprint
//////////////////////////////////////////////////////////////////////////

void CDeferredEvaluatorBlueprint::SetEvaluatorName(const char* szEvaluatorName)
{
	return Owner().SetEvaluatorName(szEvaluatorName);
}

void CDeferredEvaluatorBlueprint::SetWeight(const char* szWeight)
{
	Owner().SetWeight(szWeight);
}

const char* CDeferredEvaluatorBlueprint::GetWeight() const
{
	return Owner().GetWeight();
}

CInputBlueprint& CDeferredEvaluatorBlueprint::GetInputRoot()
{
	return Owner().GetInputRoot();
}

const CInputBlueprint& CDeferredEvaluatorBlueprint::GetInputRoot() const
{
	return Owner().GetInputRoot();
}

const char* CDeferredEvaluatorBlueprint::GetEvaluatorName() const
{
	return Owner().GetEvaluatorName();
}

//////////////////////////////////////////////////////////////////////////
// CEvaluator
//////////////////////////////////////////////////////////////////////////

void CEvaluator::SetEvaluatorName(const char* szEvaluatorName)
{
	m_name = szEvaluatorName;
}

void CEvaluator::SetWeight(const char* szWeight)
{
	m_weightString = szWeight;
	m_weight = (float)atof(szWeight);
}

const char* CEvaluator::GetWeight() const
{
	m_weightString.Format("%f", m_weight);
	return m_weightString.c_str();
}

CInputBlueprint& CEvaluator::GetInputRoot()
{
	return m_inputs;
}

const CInputBlueprint& CEvaluator::GetInputRoot() const
{
	return m_inputs;
}

const char* CEvaluator::GetEvaluatorName() const
{
	return m_name.c_str();
}

//////////////////////////////////////////////////////////////////////////

CEvaluator& SEvaluatorBlueprintAdapter::Owner() const
{
	assert(m_pOwner);
	return *m_pOwner;
}

CEvaluator CEvaluator::CreateInstant()
{
	return CEvaluator(EEvaluatorType::Instant);
}

CEvaluator CEvaluator::CreateDeferred()
{
	return CEvaluator(EEvaluatorType::Deferred);
}

CInstantEvaluatorBlueprint& CEvaluator::AsInstant()
{
	if (m_evaluatorType != EEvaluatorType::Instant)
	{
		CryFatalError("Can't cast evaluator to instant evaluator");
	}

	return *static_cast<CInstantEvaluatorBlueprint*>(m_interfaceAdapter.get());
}

CDeferredEvaluatorBlueprint& CEvaluator::AsDeferred()
{
	if (m_evaluatorType != EEvaluatorType::Deferred)
	{
		CryFatalError("Can't cast evaluator to deferred evaluator");
	}

	return *static_cast<CDeferredEvaluatorBlueprint*>(m_interfaceAdapter.get());
}

EEvaluatorType CEvaluator::GetType() const
{
	return m_evaluatorType;
}

CEvaluator::CEvaluator(const EEvaluatorType type)
	: m_name()
	, m_weight(1.0f)
	, m_weightString()
	, m_inputs()
	, m_pErrorCollector(new CErrorCollector)
	, m_evaluatorType(EEvaluatorType::Undefined)
{
	SetType(type);
}

void CEvaluator::SetType(EEvaluatorType type)
{
	m_evaluatorType = type;

	switch (type)
	{
	case EEvaluatorType::Instant:
		m_interfaceAdapter.reset(new CInstantEvaluatorBlueprint(*this));
		break;

	case EEvaluatorType::Deferred:
		m_interfaceAdapter.reset(new CDeferredEvaluatorBlueprint(*this));
		break;

	case EEvaluatorType::Undefined:
	default:
		CryFatalError("Attempt to create an evaluator of unknown type");
		break;
	}
}

CEvaluator::CEvaluator(CEvaluator&& other)
	: m_name(std::move(other.m_name))
	, m_weight(other.m_weight)
	, m_weightString(std::move(other.m_weightString))
	, m_inputs(std::move(other.m_inputs))
	, m_pErrorCollector(std::move(other.m_pErrorCollector))
	, m_evaluatorType(other.m_evaluatorType)
	, m_interfaceAdapter(std::move(other.m_interfaceAdapter))
{
	if (m_interfaceAdapter)
	{
		m_interfaceAdapter->SetOwner(*this);
	}
}

CEvaluator::CEvaluator()
	: m_name()
	, m_weight(1.0f)
	, m_weightString()
	, m_inputs()
	, m_pErrorCollector(new CErrorCollector)
	, m_evaluatorType(EEvaluatorType::Undefined)
	, m_interfaceAdapter()
{

}

CEvaluator& CEvaluator::operator=(CEvaluator&& other)
{
	if (this != &other)
	{
		m_name = std::move(other.m_name);
		m_weight = other.m_weight;
		m_weightString = std::move(other.m_weightString);
		m_inputs = std::move(other.m_inputs);
		m_pErrorCollector = std::move(other.m_pErrorCollector);
		m_evaluatorType = other.m_evaluatorType;
		m_interfaceAdapter = std::move(other.m_interfaceAdapter);
		if (m_interfaceAdapter)
		{
			m_interfaceAdapter->SetOwner(*this);
		}
	}
	return *this;
}

void CEvaluator::Serialize(Serialization::IArchive& archive)
{
	if (CUqsDocSerializationContext* pContext = archive.context<CUqsDocSerializationContext>())
	{
		assert(m_pErrorCollector);

		const bool bNameChanged = SerializeName(archive, "name", "^", *pContext);

		m_pErrorCollector->Serialize(archive, *this);

		if (!IsStringEmpty(m_name))
		{
			CEvaluatorFactoryHelper factory;

			const bool bUseSelectionHelpers = pContext->GetSettings().bUseSelectionHelpers;

			if (bUseSelectionHelpers)
			{
				factory = CEvaluatorFactoryHelper(*pContext, m_name);
			}

			if (factory.IsValid())
			{
				SetType(factory.GetType());
				if (bNameChanged)
				{
					m_inputs.ResetChildrenFromFactory(factory, *pContext);
				}
			}

			if (bNameChanged)
			{
				// Name was changed - return right now, so PropertyTree will not try
				// to insert children of previous evaluator into the new one.
				return;
			}

			const char* const szEvaluatorTypeLabel = bUseSelectionHelpers
			                                         ? "!Evaluator type"
			                                         : "Evaluator type";
			archive(m_evaluatorType, "evaluatorType", szEvaluatorTypeLabel);

			if (m_evaluatorType == EEvaluatorType::Undefined)
			{
				archive.warning(m_evaluatorType, "Undefined evaluator type");
			}

			archive(m_weight, "weight", "Weight");

			m_inputs.SerializeRoot(archive, "inputs", "Inputs", *pContext, SValidatorKey::FromObject(*this));
		}
	}
}

bool CEvaluator::SerializeName(Serialization::IArchive& archive, const char* szName, const char* szLabel, const CUqsDocSerializationContext& context)
{
	bool bNameChanged = false;

	const char* szOldName = m_name.c_str();
	auto setName = [this, &bNameChanged](const char* szNewValue)
	{
		m_name = szNewValue;
		bNameChanged = true;
	};

	if (context.GetSettings().bUseSelectionHelpers)
	{
		const Serialization::StringList& namesList = context.GetEvaluatorNamesList();

		SerializeWithStringList(archive, szName, szLabel,
		                        namesList, szOldName, setName);
	}
	else
	{
		SerializeStringWithSetter(archive, szName, szLabel, szOldName, setName);
	}
	return bNameChanged;
}

void CEvaluator::PrepareHelpers(CUqsDocSerializationContext& context)
{
	CEvaluatorFactoryHelper factory;
	CEvaluatorFactoryHelper* pFactory = nullptr;

	if (context.GetSettings().bUseSelectionHelpers)
	{
		factory = CEvaluatorFactoryHelper(context, m_name);
	}

	if (factory.IsValid())
	{
		pFactory = &factory;
	}

	m_inputs.PrepareHelpers(pFactory, context);
}

void CEvaluator::ClearErrors()
{
	assert(m_pErrorCollector);
	m_pErrorCollector->Clear();
	m_inputs.ClearErrors();
}

//////////////////////////////////////////////////////////////////////////
// CEvaluators
//////////////////////////////////////////////////////////////////////////

CInstantEvaluatorBlueprint& CEvaluators::AddInstantEvaluator()
{
	m_evaluators.emplace_back(CEvaluator::CreateInstant());
	return m_evaluators.back().AsInstant();
}

size_t CEvaluators::GetInstantEvaluatorCount() const
{
	return CalcCountOfType(EEvaluatorType::Instant);
}

CInstantEvaluatorBlueprint& CEvaluators::GetInstantEvaluator(size_t index)
{
	Evaluators::iterator iter = FindByIndexOfType(EEvaluatorType::Instant, index);
	if (iter != m_evaluators.end())
	{
		return iter->AsInstant();
	}

	CryFatalError("Index out of bounds");
	return *static_cast<CInstantEvaluatorBlueprint*>(nullptr);
}

const CInstantEvaluatorBlueprint& CEvaluators::GetInstantEvaluator(size_t index) const
{
	return const_cast<CEvaluators*>(this)->GetInstantEvaluator(index);
}

void CEvaluators::RemoveInstantEvaluator(size_t index)
{
	m_evaluators.erase(FindByIndexOfType(EEvaluatorType::Instant, index));
}

CDeferredEvaluatorBlueprint& CEvaluators::AddDeferredEvaluator()
{
	m_evaluators.emplace_back(CEvaluator::CreateDeferred());
	return m_evaluators.back().AsDeferred();
}

size_t CEvaluators::GetDeferredEvaluatorCount() const
{
	return CalcCountOfType(EEvaluatorType::Deferred);
}

CDeferredEvaluatorBlueprint& CEvaluators::GetDeferredEvaluator(size_t index)
{
	Evaluators::iterator iter = FindByIndexOfType(EEvaluatorType::Deferred, index);
	if (iter != m_evaluators.end())
	{
		return iter->AsDeferred();
	}

	CryFatalError("Index out of bounds");
	return *reinterpret_cast<CDeferredEvaluatorBlueprint*>(nullptr);
}

const CDeferredEvaluatorBlueprint& CEvaluators::GetDeferredEvaluator(size_t index) const
{
	return const_cast<CEvaluators*>(this)->GetDeferredEvaluator(index);
}

void CEvaluators::RemoveDeferredEvaluator(size_t index)
{
	m_evaluators.erase(FindByIndexOfType(EEvaluatorType::Deferred, index));
}

CEvaluators::Evaluators::iterator CEvaluators::FindByIndexOfType(const EEvaluatorType type, const size_t index)
{
	size_t idx = 0;

	const Evaluators::iterator iterEnd = m_evaluators.end();

	for (Evaluators::iterator iter = m_evaluators.begin(); iter != iterEnd; ++iter)
	{
		CEvaluator& eval = *iter;
		if (eval.GetType() == type)
		{
			if (idx == index)
			{
				return iter;
			}
			++idx;
		}
	}

	return iterEnd;
}

size_t CEvaluators::CalcCountOfType(const EEvaluatorType type) const
{
	return std::count_if(m_evaluators.begin(), m_evaluators.end(),
	                     [type](const CEvaluator& eval) { return eval.GetType() == type; });
}

void CEvaluators::Serialize(Serialization::IArchive& archive)
{
	archive(m_evaluators, "evaluators", "^");
}

void CEvaluators::PrepareHelpers(CUqsDocSerializationContext& context)
{
	for (CEvaluator& evaluator : m_evaluators)
	{
		evaluator.PrepareHelpers(context);
	}
}

void CEvaluators::ClearErrors()
{
	for (CEvaluator& evaluator : m_evaluators)
	{
		evaluator.ClearErrors();
	}
}

//////////////////////////////////////////////////////////////////////////
// SQueryFactoryType::CTraits
//////////////////////////////////////////////////////////////////////////

SQueryFactoryType::CTraits::CTraits()
	: m_pQueryFactory(nullptr)
{}

SQueryFactoryType::CTraits::CTraits(const uqs::core::IQueryFactory& queryFactory)
	: m_pQueryFactory(&queryFactory)
{}

bool SQueryFactoryType::CTraits::operator==(const CTraits& other) const
{
	if (IsUndefined() || other.IsUndefined())
		return false;

	return (RequiresGenerator() == other.RequiresGenerator())
	       && (SupportsParameters() == other.SupportsParameters())
	       && (GetMinRequiredChildren() == other.GetMinRequiredChildren())
	       && (GetMaxAllowedChildren() == other.GetMaxAllowedChildren());
}

bool SQueryFactoryType::CTraits::IsUndefined() const
{
	return (m_pQueryFactory == nullptr);
}

bool SQueryFactoryType::CTraits::RequiresGenerator() const
{
	if (m_pQueryFactory) return m_pQueryFactory->RequiresGenerator();
	return true;
}

bool SQueryFactoryType::CTraits::SupportsEvaluators() const
{
	if (m_pQueryFactory) return m_pQueryFactory->SupportsEvaluators();
	return true;
}

bool SQueryFactoryType::CTraits::SupportsParameters() const
{
	if (m_pQueryFactory) return m_pQueryFactory->SupportsParameters();
	return true;
}

size_t SQueryFactoryType::CTraits::GetMinRequiredChildren() const
{
	if (m_pQueryFactory) return m_pQueryFactory->GetMinRequiredChildren();
	return 0;
}

size_t SQueryFactoryType::CTraits::GetMaxAllowedChildren() const
{
	if (m_pQueryFactory) return m_pQueryFactory->GetMaxAllowedChildren();
	return uqs::core::IQueryFactory::kUnlimitedChildren;
}

bool SQueryFactoryType::CTraits::IsUnlimitedChildren() const
{
	return GetMaxAllowedChildren() == uqs::core::IQueryFactory::kUnlimitedChildren;
}

bool SQueryFactoryType::CTraits::CanHaveChildren() const
{
	return GetMaxAllowedChildren() > 0;
}

//////////////////////////////////////////////////////////////////////////
// SQueryFactoryType
//////////////////////////////////////////////////////////////////////////

/*static*/ const char* SQueryFactoryType::GetQueryFactoryNameByType(const EType queryFactoryType)
{
	switch (queryFactoryType)
	{
	case EType::Regular:
		return "Regular";
	case EType::Chained:
		return "Chained";
	case EType::Fallbacks:
		return "Fallbacks";
	case EType::Unknown:
		return nullptr;

	case EType::Count:
	default:
		CRY_ASSERT_MESSAGE(false, "Invalid queryFactoryType - should never get here");
		break;
	}
	static_assert((uint32)EType::Count == 4, "unexpected number of different kinds of query-factories");
	return nullptr;
}

/*static*/ SQueryFactoryType::EType SQueryFactoryType::GetQueryFactoryTypeByName(const string& queryFactoryType)
{
	static_assert((uint32)EType::Unknown + 1 == (uint32)EType::Count, "unexpected number of different kinds of query-factories");
	for (uint32 i = 0; i < (uint32)EType::Unknown; ++i)
	{
		const EType type = (EType)i;
		if (queryFactoryType == GetQueryFactoryNameByType(type))
		{
			return type;
		}
	}
	return EType::Unknown;
}

SQueryFactoryType::SQueryFactoryType(const EType queryFactoryType_)
	: queryFactoryName(GetQueryFactoryNameByType(queryFactoryType_))
	, queryFactoryType(queryFactoryType)
{}

SQueryFactoryType::SQueryFactoryType(const char* szQueryFactoryName)
	: queryFactoryName(szQueryFactoryName)
	, queryFactoryType(GetQueryFactoryTypeByName(queryFactoryName))
{}

bool SQueryFactoryType::Serialize(Serialization::IArchive& archive, const char* szName, const char* szLabel, const CUqsDocSerializationContext* pContext)
{
	bool bNameChanged = false;

	const char* szOldName = queryFactoryName.c_str();
	auto setName = [this, &bNameChanged](const char* szNewValue)
	{
		queryFactoryName = szNewValue;
		bNameChanged = true;
	};

	if (pContext && pContext->GetSettings().bUseSelectionHelpers)
	{
		const Serialization::StringList& namesList = pContext->GetQueryFactoryNamesList();

		SerializeWithStringList(archive, szName, szLabel,
		                        namesList, szOldName, setName);
	}
	else
	{
		SerializeStringWithSetter(archive, szName, szLabel, szOldName, setName);
	}

	if (bNameChanged || queryTraits.IsUndefined())
	{
		queryFactoryType = GetQueryFactoryTypeByName(queryFactoryName);
		UpdateTraits(pContext);
	}

	if (queryFactoryType == EType::Unknown && !queryFactoryName.empty())
	{
		archive.warning(szOldName, "Query factory type '%s' is unknown to the editor, helpers might be wrong.", queryFactoryName.c_str());
	}

	return bNameChanged;
}

const uqs::core::IQueryFactory* SQueryFactoryType::GetFactory(const CUqsDocSerializationContext* pContext) const
{
	if (pContext && pContext->GetSettings().bUseSelectionHelpers)
	{
		return pContext->GetQueryFactoryByName(queryFactoryName.c_str());
	}
	return nullptr;
}

void SQueryFactoryType::UpdateTraits(const CUqsDocSerializationContext* pContext)
{
	if (pContext && pContext->GetSettings().bUseSelectionHelpers)
	{
		if (const uqs::core::IQueryFactory* pQueryFactory = pContext->GetQueryFactoryByName(queryFactoryName.c_str()))
		{
			queryTraits = CTraits(*pQueryFactory);
			return;
		}
	}
	queryTraits = CTraits();
}

//////////////////////////////////////////////////////////////////////////
// CQueryBlueprint
//////////////////////////////////////////////////////////////////////////

CQueryBlueprint::CQueryBlueprint()
	: m_name()
	, m_queryFactory(SQueryFactoryType::Regular)
	, m_maxItemsToKeepInResultSet(1)
	, m_constParams()
	, m_runtimeParams()
	, m_generator()
	, m_evaluators()
	, m_queryChildren()
	, m_expectedShuttledType()
	, m_pErrorCollector(new CErrorCollector)
{
}

void CQueryBlueprint::BuildSelfFromITextualQueryBlueprint(const uqs::core::ITextualQueryBlueprint& source, CUqsDocSerializationContext& context)
{
	// name
	{
		m_name = source.GetName();
	}

	// query factory
	{
		m_queryFactory = SQueryFactoryType(source.GetQueryFactoryName());
	}

	// query children
	{
		m_queryChildren.reserve(source.GetChildCount());
		for (size_t i = 0; i < source.GetChildCount(); ++i)
		{
			const uqs::core::ITextualQueryBlueprint& childQueryBP = source.GetChild(i);
			m_queryChildren.emplace_back();
			m_queryChildren.back().BuildSelfFromITextualQueryBlueprint(childQueryBP, context);
		}
	}

	// max. items to keep in result set
	{
		const char* szMaxItems = source.GetMaxItemsToKeepInResultSet();
		m_maxItemsToKeepInResultSet = strtol(szMaxItems, nullptr, 10);
	}

	// expected shuttled type
	{
		if (const uqs::shared::IUqsString* expectedShuttledType = source.GetExpectedShuttleType())
		{
			m_expectedShuttledType.typeName = expectedShuttledType->c_str();
		}
	}

	// global constant-params
	{
		const uqs::core::ITextualGlobalConstantParamsBlueprint& sourceGlobalConstParams = source.GetGlobalConstantParams();

		for (size_t i = 0, n = sourceGlobalConstParams.GetParameterCount(); i < n; ++i)
		{
			const uqs::core::ITextualGlobalConstantParamsBlueprint::SParameterInfo pi = sourceGlobalConstParams.GetParameter(i);
			CItemLiteral value(pi.type, context);
			value.SetFromStringLiteral(CONST_TEMP_STRING(pi.value));
			m_constParams.AddParameter(pi.name, pi.type, std::move(value));
		}
	}

	// global runtime-params
	{
		const uqs::core::ITextualGlobalRuntimeParamsBlueprint& sourceGlobalRuntimeParams = source.GetGlobalRuntimeParams();

		for (size_t i = 0, n = sourceGlobalRuntimeParams.GetParameterCount(); i < n; ++i)
		{
			const uqs::core::ITextualGlobalRuntimeParamsBlueprint::SParameterInfo pi = sourceGlobalRuntimeParams.GetParameter(i);
			m_runtimeParams.AddParameter(pi.name, pi.type);
		}
	}

	// generator (optional and typically never used by composite queries)
	{
		if (const uqs::core::ITextualGeneratorBlueprint* pSourceGeneratorBlueprint = source.GetGenerator())
		{
			m_generator.SetGeneratorName(pSourceGeneratorBlueprint->GetGeneratorName());

			HelpBuildCInputBlueprintHierarchyFromITextualInputBlueprint(m_generator.GetInputRoot(), pSourceGeneratorBlueprint->GetInputRoot());
		}
	}

	// instant-evaluators
	{
		for (size_t i = 0, n = source.GetInstantEvaluatorCount(); i < n; ++i)
		{
			const uqs::core::ITextualInstantEvaluatorBlueprint& sourceInstantEvaluatorBlueprint = source.GetInstantEvaluator(i);
			CInstantEvaluatorBlueprint& targetInstantEvaluatorBlueprint = m_evaluators.AddInstantEvaluator();

			targetInstantEvaluatorBlueprint.SetEvaluatorName(sourceInstantEvaluatorBlueprint.GetEvaluatorName());
			targetInstantEvaluatorBlueprint.SetWeight(sourceInstantEvaluatorBlueprint.GetWeight());

			HelpBuildCInputBlueprintHierarchyFromITextualInputBlueprint(targetInstantEvaluatorBlueprint.GetInputRoot(), sourceInstantEvaluatorBlueprint.GetInputRoot());
		}
	}

	// deferred-evaluators
	{
		for (size_t i = 0, n = source.GetDeferredEvaluatorCount(); i < n; ++i)
		{
			const uqs::core::ITextualDeferredEvaluatorBlueprint& sourceDeferredEvaluatorBlueprint = source.GetDeferredEvaluator(i);
			CDeferredEvaluatorBlueprint& targetDeferredEvaluatorBlueprint = m_evaluators.AddDeferredEvaluator();

			targetDeferredEvaluatorBlueprint.SetEvaluatorName(sourceDeferredEvaluatorBlueprint.GetEvaluatorName());
			targetDeferredEvaluatorBlueprint.SetWeight(sourceDeferredEvaluatorBlueprint.GetWeight());

			HelpBuildCInputBlueprintHierarchyFromITextualInputBlueprint(targetDeferredEvaluatorBlueprint.GetInputRoot(), sourceDeferredEvaluatorBlueprint.GetInputRoot());
		}
	}
}

void CQueryBlueprint::BuildITextualQueryBlueprintFromSelf(uqs::core::ITextualQueryBlueprint& target) const
{
	target.SetSyntaxErrorCollector(MakeNewSyntaxErrorCollectorUniquePtr(m_pErrorCollector));

	// name
	{
		target.SetName(m_name.c_str());
	}

	// query factory
	{
		target.SetQueryFactoryName(m_queryFactory.queryFactoryName.c_str());
	}

	// query children
	{
		for (const CQueryBlueprint& childQuery : m_queryChildren)
		{
			uqs::core::ITextualQueryBlueprint& targetChild = target.AddChild();
			childQuery.BuildITextualQueryBlueprintFromSelf(targetChild);
		}
	}

	// max. items to keep in result set
	{
		stack_string s;
		s.Format("%" PRISIZE_T, m_maxItemsToKeepInResultSet);
		target.SetMaxItemsToKeepInResultSet(s.c_str());
	}

	if (!m_expectedShuttledType.Empty())
	{
		target.SetExpectedShuttleType(m_expectedShuttledType.c_str());
	}

	// global constant-params
	{
		uqs::core::ITextualGlobalConstantParamsBlueprint& targetConstParams = target.GetGlobalConstantParams();

		string value;
		for (size_t i = 0, n = m_constParams.GetParameterCount(); i < n; ++i)
		{
			const char* szName;
			const char* szType;
			value.clear();
			std::shared_ptr<CErrorCollector> pErrorCollector;
			m_constParams.GetParameterInfo(i, szName, szType, *&value, pErrorCollector);
			targetConstParams.AddParameter(szName, szType, value.c_str(), MakeNewSyntaxErrorCollectorUniquePtr(pErrorCollector));
		}
	}

	// global runtime-params
	{
		uqs::core::ITextualGlobalRuntimeParamsBlueprint& targetRuntimeParams = target.GetGlobalRuntimeParams();

		for (size_t i = 0, n = m_runtimeParams.GetParameterCount(); i < n; ++i)
		{
			const char* szName;
			const char* szType;
			std::shared_ptr<CErrorCollector> pErrorCollector;
			m_runtimeParams.GetParameterInfo(i, szName, szType, pErrorCollector);
			targetRuntimeParams.AddParameter(szName, szType, MakeNewSyntaxErrorCollectorUniquePtr(pErrorCollector));
		}
	}

	// generator
	if (m_generator.IsSet())
	{
		uqs::core::ITextualGeneratorBlueprint& targetGeneratorBlueprint = target.SetGenerator();

		targetGeneratorBlueprint.SetGeneratorName(m_generator.GetGeneratorName());
		targetGeneratorBlueprint.SetSyntaxErrorCollector(MakeNewSyntaxErrorCollectorUniquePtr(m_generator.GetErrorCollectorSharedPtr()));

		HelpBuildITextualInputBlueprintHierarchyFromCInputBlueprint(targetGeneratorBlueprint.GetInputRoot(), m_generator.GetInputRoot());
	}

	// instant-evaluators
	{
		for (size_t i = 0, n = m_evaluators.GetInstantEvaluatorCount(); i < n; ++i)
		{
			const CInstantEvaluatorBlueprint& sourceInstantEvaluatorBlueprint = m_evaluators.GetInstantEvaluator(i);
			uqs::core::ITextualInstantEvaluatorBlueprint& targetInstantEvaluatorBlueprint = target.AddInstantEvaluator();

			targetInstantEvaluatorBlueprint.SetEvaluatorName(sourceInstantEvaluatorBlueprint.GetEvaluatorName());
			targetInstantEvaluatorBlueprint.SetWeight(sourceInstantEvaluatorBlueprint.GetWeight());
			targetInstantEvaluatorBlueprint.SetSyntaxErrorCollector(MakeNewSyntaxErrorCollectorUniquePtr(sourceInstantEvaluatorBlueprint.Owner().GetErrorCollectorSharedPtr()));

			HelpBuildITextualInputBlueprintHierarchyFromCInputBlueprint(targetInstantEvaluatorBlueprint.GetInputRoot(), sourceInstantEvaluatorBlueprint.GetInputRoot());
		}
	}

	// deferred-evaluators
	{
		for (size_t i = 0, n = m_evaluators.GetDeferredEvaluatorCount(); i < n; ++i)
		{
			const CDeferredEvaluatorBlueprint& sourceDeferredEvaluatorBlueprint = m_evaluators.GetDeferredEvaluator(i);
			uqs::core::ITextualDeferredEvaluatorBlueprint& targetDeferredEvaluatorBlueprint = target.AddDeferredEvaluator();

			targetDeferredEvaluatorBlueprint.SetEvaluatorName(sourceDeferredEvaluatorBlueprint.GetEvaluatorName());
			targetDeferredEvaluatorBlueprint.SetWeight(sourceDeferredEvaluatorBlueprint.GetWeight());
			targetDeferredEvaluatorBlueprint.SetSyntaxErrorCollector(MakeNewSyntaxErrorCollectorUniquePtr(sourceDeferredEvaluatorBlueprint.Owner().GetErrorCollectorSharedPtr()));

			HelpBuildITextualInputBlueprintHierarchyFromCInputBlueprint(targetDeferredEvaluatorBlueprint.GetInputRoot(), sourceDeferredEvaluatorBlueprint.GetInputRoot());
		}
	}
}

void CQueryBlueprint::HelpBuildCInputBlueprintHierarchyFromITextualInputBlueprint(CInputBlueprint& targetRoot, const uqs::core::ITextualInputBlueprint& sourceRoot)
{
	for (size_t i = 0, n = sourceRoot.GetChildCount(); i < n; ++i)
	{
		const uqs::core::ITextualInputBlueprint& sourceChild = sourceRoot.GetChild(i);
		const char* szParamName = sourceChild.GetParamName();
		const char* szFuncName = sourceChild.GetFuncName();
		const char* szFuncReturnValueLiteral = sourceChild.GetFuncReturnValueLiteral();
		const char* szAddReturnValueToDebugRenderWorldUponExecution = sourceChild.GetAddReturnValueToDebugRenderWorldUponExecution();

		CInputBlueprint& newChild = targetRoot.AddChild(szParamName, szFuncName, szFuncReturnValueLiteral, szAddReturnValueToDebugRenderWorldUponExecution);

		// recurse down
		HelpBuildCInputBlueprintHierarchyFromITextualInputBlueprint(newChild, sourceChild);
	}
}

void CQueryBlueprint::HelpBuildITextualInputBlueprintHierarchyFromCInputBlueprint(uqs::core::ITextualInputBlueprint& targetRoot, const CInputBlueprint& sourceRoot)
{
	for (size_t i = 0, n = sourceRoot.GetChildCount(); i < n; ++i)
	{
		const CInputBlueprint& sourceChild = sourceRoot.GetChild(i);
		const char* szParamName = sourceChild.GetParamName();
		const char* szFuncName = sourceChild.GetFuncName();
		const char* szFuncReturnValueLiteral = sourceChild.GetFuncReturnValueLiteral();
		const char* szAddReturnValueToDebugRenderWorldUponExecution = sourceChild.GetAddReturnValueToDebugRenderWorldUponExecution();
		const std::shared_ptr<CErrorCollector>& pErrorCollector = sourceChild.GetErrorCollectorSharedPtr();

		uqs::core::ITextualInputBlueprint& newChild = targetRoot.AddChild(szParamName, szFuncName, szFuncReturnValueLiteral, szAddReturnValueToDebugRenderWorldUponExecution);
		newChild.SetSyntaxErrorCollector(MakeNewSyntaxErrorCollectorUniquePtr(pErrorCollector));

		// recurse down
		HelpBuildITextualInputBlueprintHierarchyFromCInputBlueprint(newChild, sourceChild);
	}
}

void CQueryBlueprint::CheckQueryTraitsChange(const SQueryFactoryType::CTraits& queryTraits, const SQueryFactoryType::CTraits& oldTraits, CParametersListContext& paramListContext)
{
	if (oldTraits != queryTraits && !oldTraits.IsUndefined() && !queryTraits.IsUndefined())
	{
		if (oldTraits.SupportsParameters() != queryTraits.SupportsParameters())
		{
			paramListContext.SetParamsChanged(true);
			if (!queryTraits.SupportsParameters())
			{
				m_constParams = CConstParamBlueprint();
				m_runtimeParams = CRuntimeParamBlueprint();
			}
		}
		if (oldTraits.RequiresGenerator() != queryTraits.RequiresGenerator())
		{
			if (!queryTraits.RequiresGenerator())
			{
				m_generator = CGeneratorBlueprint();
			}
		}
		if (oldTraits.SupportsEvaluators() != queryTraits.SupportsEvaluators())
		{
			if (!queryTraits.SupportsEvaluators())
			{
				m_evaluators = CEvaluators();
			}
		}
		if (oldTraits.CanHaveChildren() != queryTraits.CanHaveChildren())
		{
			if (!queryTraits.CanHaveChildren())
			{
				m_queryChildren = QueryBlueprintChildren();
			}
		}
	}
}

void CQueryBlueprint::Serialize(Serialization::IArchive& archive)
{
	assert(m_pErrorCollector);

	m_pErrorCollector->Serialize(archive, *this);

	CUqsDocSerializationContext* pContext = archive.context<CUqsDocSerializationContext>();

	if (!m_name.empty())
	{
		archive(m_name, "name", "!Name");
	}

	const SQueryFactoryType::CTraits oldTraits = m_queryFactory.GetTraits();
	m_queryFactory.Serialize(archive, "queryFactory", "^Query Factory", pContext);
	const SQueryFactoryType::CTraits& queryTraits = m_queryFactory.GetTraits();

	CParametersListContext paramListContext(this, pContext);

	CheckQueryTraitsChange(queryTraits, oldTraits, paramListContext);

	archive(m_maxItemsToKeepInResultSet, "maxItemsToKeep", "Max items to keep in result set");
	archive(m_expectedShuttledType, "expectedShuttledType", "Shuttled Type");

	if (queryTraits.SupportsParameters())
	{
		archive(m_constParams, "constParams", "Constant parameters");
		archive(m_runtimeParams, "runtimeParams", "Runtime parameters");
	}

	if (queryTraits.CanHaveChildren())
	{
		archive(m_queryChildren, "queryChildren", "Query Children");
	}

	if (queryTraits.RequiresGenerator() || queryTraits.SupportsEvaluators())
	{
		CSelectedGeneratorContext selectedGeneratorContext(this, pContext);

		if (paramListContext.GetParamsChanged())
		{
			assert(archive.isInput());
			// Parameters were changed - stop input serialization now or else PropertyTree may discard some references to parameters.
			// TODO pavloi 2016.04.25: #FIX_COPY

			if (pContext)
			{
				m_generator.PrepareHelpers(*pContext);
				selectedGeneratorContext.SetGeneratorProcessed(true);
				m_evaluators.PrepareHelpers(*pContext);
			}

			return;
		}

		if (queryTraits.RequiresGenerator())
		{
			archive(m_generator, "generator", "Generator");
			selectedGeneratorContext.SetGeneratorProcessed(true);
		}

		if (selectedGeneratorContext.GetGeneratorChanged())
		{
			assert(archive.isInput());
			// Parameters were changed - stop input serialization now or else PropertyTree may discard some references to parameters.
			// TODO pavloi 2016.04.25: #FIX_COPY

			if (pContext)
			{
				m_evaluators.PrepareHelpers(*pContext);
			}
			return;
		}

		if (queryTraits.SupportsEvaluators())
		{
			archive(m_evaluators, "evaluators", "Evaluators");
		}
	}
}

void CQueryBlueprint::PrepareHelpers(CUqsDocSerializationContext& context)
{
	m_queryFactory.UpdateTraits(&context);

	CParametersListContext paramListContext(this, &context);

	m_constParams.PrepareHelpers(context);
	m_runtimeParams.PrepareHelpers(context);

	for (CQueryBlueprint& child : m_queryChildren)
	{
		child.PrepareHelpers(context);
	}

	CSelectedGeneratorContext selectedGeneratorContext(this, &context);
	m_generator.PrepareHelpers(context);

	selectedGeneratorContext.SetGeneratorProcessed(true);

	m_evaluators.PrepareHelpers(context);
}

void CQueryBlueprint::ClearErrors()
{
	assert(m_pErrorCollector);
	m_pErrorCollector->Clear();
	m_constParams.ClearErrors();
	m_runtimeParams.ClearErrors();
	m_generator.ClearErrors();
	m_evaluators.ClearErrors();

	for (CQueryBlueprint& child : m_queryChildren)
	{
		child.ClearErrors();
	}
}

const CGeneratorBlueprint& CQueryBlueprint::GetGenerator() const
{
	return m_generator;
}

//////////////////////////////////////////////////////////////////////////
// CParametersListContext
//////////////////////////////////////////////////////////////////////////

CParametersListContext::CParametersListContext(CQueryBlueprint* pOwner, CUqsDocSerializationContext* pSerializationContext)
	: m_pSerializationContext(pSerializationContext)
	, m_pOwner(pOwner)
	, m_bParamsChanged(false)
{
	if (pSerializationContext)
	{
		pSerializationContext->PushParametersListContext(this);
	}
}

CParametersListContext::~CParametersListContext()
{
	if (m_pSerializationContext)
	{
		m_pSerializationContext->PopParametersListContext(this);
	}
}

void CParametersListContext::BuildFunctionListForAvailableParameters(
  const SItemTypeName& typeNameToFilter,
  const CUqsDocSerializationContext& context,
  const std::vector<CFunctionSerializationHelper::SFunction>& allGlobalParamFunctions,
  std::vector<CFunctionSerializationHelper::SFunction>& outParamFunctions)
{
	outParamFunctions.clear();
	assert(m_pOwner);
	if (!m_pOwner)
	{
		return;
	}

	const size_t constParamsCount = m_pOwner->m_constParams.m_params.size();
	const size_t runtimeParamsCount = m_pOwner->m_runtimeParams.m_params.size();

	const size_t count = constParamsCount + runtimeParamsCount;
	outParamFunctions.reserve(count);

	const bool bApplyFilter = !typeNameToFilter.Empty() && context.GetSettings().bFilterAvailableInputsByType;

	for (const CConstParamBlueprint::SConstParam& param : m_pOwner->m_constParams.m_params)
	{
		if (param.IsValid())
		{
			if (!bApplyFilter || (param.type == typeNameToFilter))
			{
				for (const auto& func : allGlobalParamFunctions)
				{
					assert(func.pFactory);
					if (func.returnType == param.type)
					{
						outParamFunctions.emplace_back(func);
						auto& outFunc = outParamFunctions.back();
						outFunc.param = param.name;
					}
				}
			}
		}
	}

	for (const CRuntimeParamBlueprint::SRuntimeParam& param : m_pOwner->m_runtimeParams.m_params)
	{
		if (param.IsValid())
		{
			if (!bApplyFilter || (param.type == typeNameToFilter))
			{
				for (const auto& func : allGlobalParamFunctions)
				{
					assert(func.pFactory);
					if (func.returnType == param.type)
					{
						outParamFunctions.emplace_back(func);
						auto& outFunc = outParamFunctions.back();
						outFunc.param = param.name;
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// CSelectedGeneratorContext
//////////////////////////////////////////////////////////////////////////

CSelectedGeneratorContext::CSelectedGeneratorContext(CQueryBlueprint* pOwner, CUqsDocSerializationContext* pSerializationContext)
	: m_pSerializationContext(pSerializationContext)
	, m_pOwner(pOwner)
	, m_bGeneratorChanged(false)
	, m_bGeneratorProcessed(false)
{
	if (pSerializationContext)
	{
		pSerializationContext->PushSelectedGeneratorContext(this);
	}
}

CSelectedGeneratorContext::~CSelectedGeneratorContext()
{
	if (m_pSerializationContext)
	{
		m_pSerializationContext->PopSelectedGeneratorContext(this);
	}
}

void CSelectedGeneratorContext::BuildFunctionListForAvailableGenerators(
  const SItemTypeName& typeNameToFilter,
  const CUqsDocSerializationContext& context,
  const std::vector<CFunctionSerializationHelper::SFunction>& allIteraedItemFunctions,
  std::vector<CFunctionSerializationHelper::SFunction>& outParamFunctions)
{
	outParamFunctions.clear();
	assert(m_pOwner);
	if (!m_pOwner)
	{
		return;
	}

	const bool bApplyFilter = !typeNameToFilter.Empty() && context.GetSettings().bFilterAvailableInputsByType;

	if (bApplyFilter)
	{
		SItemTypeName generatorType;
		const char* szGeneratorName = m_pOwner->GetGenerator().GetGeneratorName();
		if (uqs::client::IGeneratorFactory* pFactory = context.GetGeneratorFactoryByName(szGeneratorName))
		{
			generatorType = context.GetItemTypeNameFromType(pFactory->GetTypeOfItemsToGenerate());
		}

		if (generatorType != typeNameToFilter)
		{
			return;
		}
	}

	for (const auto& func : allIteraedItemFunctions)
	{
		if (!bApplyFilter || (func.returnType == typeNameToFilter))
		{
			outParamFunctions.emplace_back(func);
		}
	}
}

} // namespace uqseditor
