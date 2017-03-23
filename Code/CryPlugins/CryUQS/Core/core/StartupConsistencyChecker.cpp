// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "StartupConsistencyChecker.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Core
	{

		void CStartupConsistencyChecker::CheckForConsistencyErrors()
		{
			//
			// clear previous errors so that user can correct some of the problems while editing a query and then re-check for consistency
			//

			m_errors.clear();

			//
			// ensure no duplicate item factories (same item type)
			//

			{
				const ItemFactoryDatabase& itemFactoryDB = g_pHub->GetItemFactoryDatabase();

				for (size_t i1 = 0, n = itemFactoryDB.GetFactoryCount(); i1 < n; ++i1)
				{
					const Client::IItemFactory& itemFactory1 = itemFactoryDB.GetFactory(i1);
					const Shared::CTypeInfo& itemType1 = itemFactory1.GetItemType();
					int duplicateCount = 0;
					stack_string typeNamesForErrorMessage;

					for (size_t i2 = i1 + 1; i2 < n; ++i2)
					{
						const Client::IItemFactory& itemFactory2 = itemFactoryDB.GetFactory(i2);
						const Shared::CTypeInfo& itemType2 = itemFactory2.GetItemType();

						if (itemType1 == itemType2)
						{
							++duplicateCount;

							if (duplicateCount == 1)
							{
								typeNamesForErrorMessage = typeNamesForErrorMessage + "'" + itemFactory1.GetName() + "'";
							}

							typeNamesForErrorMessage = typeNamesForErrorMessage + ", '" + itemFactory2.GetName() + "'";
						}
					}

					if (duplicateCount > 0)
					{
						string error;
						error.Format("Duplicate ItemFactory that creates items of type %s (you registered %i such item-factories under these names: %s)", itemType1.name(), duplicateCount + 1, typeNamesForErrorMessage.c_str());
						m_errors.push_back(error);
					}
				}
			}

#if UQS_SCHEMATYC_SUPPORT
			//
			// ensure no duplicated and clashing item factory GUIDs
			//

			{
				const ItemFactoryDatabase& itemFactoryDB = g_pHub->GetItemFactoryDatabase();

				for (size_t i1 = 0, n = itemFactoryDB.GetFactoryCount(); i1 < n; ++i1)
				{
					static const CryGUID emptyGUID = CryGUID::Null();

					const Client::IItemFactory& itemFactory1 = itemFactoryDB.GetFactory(i1);
					const CryGUID& guid1ofItemFactory1 = itemFactory1.GetGUIDForSchematycAddParamFunction();
					const CryGUID& guid2ofItemFactory1 = itemFactory1.GetGUIDForSchematycGetItemFromResultSetFunction();
					
					//
					// itemFactory1: skip if both of its GUIDs are empty
					//
					
					if (guid1ofItemFactory1 == emptyGUID && guid2ofItemFactory1 == emptyGUID)
					{
						continue;
					}

					//
					// itemFactory1: check for one GUID being empty while the other isn't
					//

					if ((guid1ofItemFactory1 == emptyGUID && guid2ofItemFactory1 != emptyGUID) ||
					    (guid1ofItemFactory1 != emptyGUID && guid2ofItemFactory1 == emptyGUID))
					{
						string error;
						error.Format("ItemFactory '%s' has inconsistent GUIDs: one is empty while the other is not", itemFactory1.GetName());
						m_errors.push_back(error);
						continue;
					}

					//
					// itemFactory1: its 2 guids are non-empty -> check for duplicate
					//

					if (guid1ofItemFactory1 == guid2ofItemFactory1)
					{
						string error;
						error.Format("ItemFactory '%s' has duplicated GUIDs", itemFactory1.GetName());
						m_errors.push_back(error);
						continue;
					}

					//
					// now check the remaining item-factories for clashes
					//

					for (size_t i2 = i1 + 1; i2 < n; ++i2)
					{
						const Client::IItemFactory& itemFactory2 = itemFactoryDB.GetFactory(i2);
						const CryGUID& guid1ofItemFactory2 = itemFactory2.GetGUIDForSchematycAddParamFunction();
						const CryGUID& guid2ofItemFactory2 = itemFactory2.GetGUIDForSchematycGetItemFromResultSetFunction();

						if (guid1ofItemFactory1 == guid1ofItemFactory2 ||
						    guid2ofItemFactory1 == guid2ofItemFactory2 ||
						    guid1ofItemFactory1 == guid2ofItemFactory2 ||
						    guid2ofItemFactory1 == guid1ofItemFactory2)
						{
							string error;
							error.Format("Clashing GUIDs in ItemFactories '%s' and '%s'", itemFactory1.GetName(), itemFactory2.GetName());
							m_errors.push_back(error);
						}
					}
				}
			}

			//
			// ensure no duplicate GUIDs in item converters
			//

			{
				std::set<CryGUID> guidsInUse;

				const ItemFactoryDatabase& itemFactoryDB = g_pHub->GetItemFactoryDatabase();

				for (size_t iItemFactory = 0; iItemFactory < itemFactoryDB.GetFactoryCount(); ++iItemFactory)
				{
					const Client::IItemFactory& itemFactory = itemFactoryDB.GetFactory(iItemFactory);
					const char* szItemFactoryNameForErrorMessages = itemFactory.GetName();

					CheckItemConvertersConsistency(itemFactory.GetFromForeignTypeConverters(), szItemFactoryNameForErrorMessages, guidsInUse);
					CheckItemConvertersConsistency(itemFactory.GetToForeignTypeConverters(), szItemFactoryNameForErrorMessages, guidsInUse);
				}
			}
#endif // UQS_SCHEMATYC_SUPPORT

			//
			// ensure no duplicate item factories (same name)
			//

			{
				const std::map<string, int> duplicateItemFactoryNames = g_pHub->GetItemFactoryDatabase().GetDuplicateFactoryNames();

				if (!duplicateItemFactoryNames.empty())
				{
					for (const auto& pair : duplicateItemFactoryNames)
					{
						string error;
						error.Format("Duplicate ItemFactory: '%s' (you tried to register an item-factory under this name %i times)", pair.first.c_str(), pair.second);
						m_errors.push_back(error);
					}
				}
			}

			//
			// ensure no duplicate function factories
			//

			{
				const std::map<string, int> duplicateFunctionFactoryNames = g_pHub->GetFunctionFactoryDatabase().GetDuplicateFactoryNames();

				if (!duplicateFunctionFactoryNames.empty())
				{
					for (const auto& pair : duplicateFunctionFactoryNames)
					{
						string error;
						error.Format("Duplicate FunctionFactory: '%s' (you tried to register a function-factory under this name %i times)", pair.first.c_str(), pair.second);
						m_errors.push_back(error);
					}
				}
			}

			//
			// ensure no duplicate generator factories
			//

			{
				const std::map<string, int> duplicateGeneratorFactoryNames = g_pHub->GetGeneratorFactoryDatabase().GetDuplicateFactoryNames();

				if (!duplicateGeneratorFactoryNames.empty())
				{
					for (const auto& pair : duplicateGeneratorFactoryNames)
					{
						string error;
						error.Format("Duplicate GeneratorFactory: '%s' (you tried to register a generator-factory under this name %i times)", pair.first.c_str(), pair.second);
						m_errors.push_back(error);
					}
				}
			}

			//
			// ensure no duplicate instant-evaluator factories
			//

			{
				const std::map<string, int> duplicateInstantEvaluatorFactoryNames = g_pHub->GetInstantEvaluatorFactoryDatabase().GetDuplicateFactoryNames();

				if (!duplicateInstantEvaluatorFactoryNames.empty())
				{
					for (const auto& pair : duplicateInstantEvaluatorFactoryNames)
					{
						string error;
						error.Format("Duplicate InstantEvaluatorFactory: '%s' (you tried to register an instant-evaluator-factory under this name %i times)", pair.first.c_str(), pair.second);
						m_errors.push_back(error);
					}
				}
			}

			//
			// ensure no duplicate deferred-evaluator factories
			//

			{
				const std::map<string, int> duplicateDeferredEvaluatorFactoryNames = g_pHub->GetDeferredEvaluatorFactoryDatabase().GetDuplicateFactoryNames();

				if (!duplicateDeferredEvaluatorFactoryNames.empty())
				{
					for (const auto& pair : duplicateDeferredEvaluatorFactoryNames)
					{
						string error;
						error.Format("Duplicate DeferredEvaluatorFactory: '%s' (you tried to register a deferred-evaluator-factory under this name %i times)", pair.first.c_str(), pair.second);
						m_errors.push_back(error);
					}
				}
			}

			//
			// ensure that all generators generate items for which item-factories are registered
			//

			{
				const GeneratorFactoryDatabase& generatorFactoryDB = g_pHub->GetGeneratorFactoryDatabase();

				for (size_t i = 0, n = generatorFactoryDB.GetFactoryCount(); i < n; ++i)
				{
					const Client::IGeneratorFactory& generatorFactory = generatorFactoryDB.GetFactory(i);
					const Shared::CTypeInfo& itemTypeToGenerate = generatorFactory.GetTypeOfItemsToGenerate();

					if (!g_pHub->GetUtils().FindItemFactoryByType(itemTypeToGenerate))
					{
						string error;
						error.Format("Generator '%s' wants to generate items of type '%s', but there is no corresponding item-factory for this type registered", generatorFactory.GetName(), itemTypeToGenerate.name());
						m_errors.push_back(error);
					}
				}
			}

			//
			// ensure that all kinds of return types of functions have a corresponding item-factory registered
			//

			{
				const FunctionFactoryDatabase& functionFactoryDB = g_pHub->GetFunctionFactoryDatabase();

				for (size_t i = 0, n = functionFactoryDB.GetFactoryCount(); i < n; ++i)
				{
					const Client::IFunctionFactory& functionFactory = functionFactoryDB.GetFactory(i);
					const Shared::CTypeInfo& returnTypeOfFunction = functionFactory.GetReturnType();

					if (!g_pHub->GetUtils().FindItemFactoryByType(returnTypeOfFunction))
					{
						string error;
						error.Format("Function '%s' returns a '%s', but there is no corresponding item-factory for this type registered", functionFactory.GetName(), returnTypeOfFunction.name());
						m_errors.push_back(error);
					}
				}
			}

			//
			// ensure that all input parameters have known item types, unique names and unique memory offsets
			//
			// -> 1. function params
			// -> 2. generator params
			// -> 3. instant-evaluator params
			// -> 4. deferred-evaluator params
			//

			{
				// 1. function params
				{
					const FunctionFactoryDatabase& functionFactoryDB = g_pHub->GetFunctionFactoryDatabase();

					for (size_t i = 0, n = functionFactoryDB.GetFactoryCount(); i < n; ++i)
					{
						const Client::IFunctionFactory& functionFactory = functionFactoryDB.GetFactory(i);
						stack_string messagePrefixForPossibleError;
						messagePrefixForPossibleError.Format("Function '%s'", functionFactory.GetName());
						CheckInputParametersConsistency(functionFactory.GetInputParameterRegistry(), messagePrefixForPossibleError.c_str());
					}
				}

				// 2. generator params
				{
					const GeneratorFactoryDatabase& generatorFactoryDB = g_pHub->GetGeneratorFactoryDatabase();

					for (size_t i = 0, n = generatorFactoryDB.GetFactoryCount(); i < n; ++i)
					{
						const Client::IGeneratorFactory& generatorFactory = generatorFactoryDB.GetFactory(i);
						stack_string messagePrefixForPossibleError;
						messagePrefixForPossibleError.Format("Generator '%s'", generatorFactory.GetName());
						CheckInputParametersConsistency(generatorFactory.GetInputParameterRegistry(), messagePrefixForPossibleError.c_str());
					}
				}

				// 3. instant-evaluator params
				{
					const IInstantEvaluatorFactoryDatabase& ieFactoryDB = g_pHub->GetInstantEvaluatorFactoryDatabase();

					for (size_t i = 0, n = ieFactoryDB.GetFactoryCount(); i < n; ++i)
					{
						const Client::IInstantEvaluatorFactory& ieFactory = ieFactoryDB.GetFactory(i);
						stack_string messagePrefixForPossibleError;
						messagePrefixForPossibleError.Format("Instant-Evaluator '%s'", ieFactory.GetName());
						CheckInputParametersConsistency(ieFactory.GetInputParameterRegistry(), messagePrefixForPossibleError.c_str());
					}
				}

				// 4. deferred-evaluator params
				{
					const IDeferredEvaluatorFactoryDatabase& deFactoryDB = g_pHub->GetDeferredEvaluatorFactoryDatabase();

					for (size_t i = 0, n = deFactoryDB.GetFactoryCount(); i < n; ++i)
					{
						const Client::IDeferredEvaluatorFactory& deFactory = deFactoryDB.GetFactory(i);
						stack_string messagePrefixForPossibleError;
						messagePrefixForPossibleError.Format("Deferred-Evaluator '%s'", deFactory.GetName());
						CheckInputParametersConsistency(deFactory.GetInputParameterRegistry(), messagePrefixForPossibleError.c_str());
					}
				}
			}

			// TODO: more consistency checks
		}

		void CStartupConsistencyChecker::CheckInputParametersConsistency(const Client::IInputParameterRegistry& registry, const char* szErrorMessagePrefix)
		{
			for (size_t i = 0, n = registry.GetParameterCount(); i < n; ++i)
			{
				const Client::IInputParameterRegistry::SParameterInfo& pi = registry.GetParameter(i);

				// check for known item type
				if (g_pHub->GetUtils().FindItemFactoryByType(pi.type) == nullptr)
				{
					string error;
					error.Format("%s: parameter '%s' is of type '%s' but there is no such item-factory registered", szErrorMessagePrefix, pi.szName, pi.type.name());
					m_errors.push_back(error);
				}

				// check for unique name (prevent copy & paste error)
				for (size_t k = i + 1; k < n; ++k)
				{
					const Client::IInputParameterRegistry::SParameterInfo& pi2 = registry.GetParameter(k);
					if (strcmp(pi2.szName, pi.szName) == 0)
					{
						string error;
						error.Format("%s: duplicate parameter name: '%s' (could be a copy & paste error)", szErrorMessagePrefix, pi2.szName);
						m_errors.push_back(error);
					}
				}

				// check for unique memory offset (prevent copy & paste error)
				for (size_t k = i + 1; k < n; ++k)
				{
					const Client::IInputParameterRegistry::SParameterInfo& pi2 = registry.GetParameter(k);
					if (pi2.offset == pi.offset)
					{
						string error;
						error.Format("%s: duplicate memory offset for parameter '%s': the parameter '%s' is already using this offset (could be a copy & paste error)", szErrorMessagePrefix, pi2.szName, pi.szName);
						m_errors.push_back(error);
					}
				}
			}
		}

#if UQS_SCHEMATYC_SUPPORT
		void CStartupConsistencyChecker::CheckItemConvertersConsistency(const Client::IItemConverterCollection& itemConverters, const char* szItemFactoryNameForErrorMessages, std::set<CryGUID>& guidsInUse)
		{
			for (size_t i = 0, n = itemConverters.GetItemConverterCount(); i < n; ++i)
			{
				const Client::IItemConverter& converter = itemConverters.GetItemConverter(i);

				const CryGUID& guid = converter.GetGUID();
				auto res = guidsInUse.insert(guid);
				const bool bInsertSucceeded = res.second;

				if (!bInsertSucceeded)
				{
					// duplicate detected
					Shared::CUqsString guidAsString;
					Client::Internal::CGUIDHelper::ToString(guidAsString, guid);
					string error;
					error.Format("ItemFactory '%s': clashing GUID in ItemConverter '%s -> %s': %s (used by a different ItemConverter already)", szItemFactoryNameForErrorMessages, converter.GetFromName(), converter.GetToName(), guidAsString.c_str());
					m_errors.push_back(error);
				}
			}
		}
#endif // UQS_SCHEMATYC_SUPPORT

		size_t CStartupConsistencyChecker::GetErrorCount() const
		{
			return m_errors.size();
		}

		const char* CStartupConsistencyChecker::GetError(size_t index) const
		{
			assert(index < m_errors.size());
			return m_errors[index].c_str();
		}

	}
}
