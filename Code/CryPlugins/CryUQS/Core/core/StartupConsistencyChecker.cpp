// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "StartupConsistencyChecker.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
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
				const ItemFactoryDatabase& itemFactoryDB = g_hubImpl->GetItemFactoryDatabase();

				for (size_t i1 = 0, n = itemFactoryDB.GetFactoryCount(); i1 < n; ++i1)
				{
					const client::IItemFactory& itemFactory1 = itemFactoryDB.GetFactory(i1);
					const shared::CTypeInfo& itemType1 = itemFactory1.GetItemType();
					int duplicateCount = 0;
					stack_string typeNamesForErrorMessage;

					for (size_t i2 = i1 + 1; i2 < n; ++i2)
					{
						const client::IItemFactory& itemFactory2 = itemFactoryDB.GetFactory(i2);
						const shared::CTypeInfo& itemType2 = itemFactory2.GetItemType();

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

			//
			// ensure no duplicate item factories (same name)
			//

			{
				const std::map<string, int> duplicateItemFactoryNames = g_hubImpl->GetItemFactoryDatabase().GetDuplicateFactoryNames();

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
				const std::map<string, int> duplicateFunctionFactoryNames = g_hubImpl->GetFunctionFactoryDatabase().GetDuplicateFactoryNames();

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
				const std::map<string, int> duplicateGeneratorFactoryNames = g_hubImpl->GetGeneratorFactoryDatabase().GetDuplicateFactoryNames();

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
				const std::map<string, int> duplicateInstantEvaluatorFactoryNames = g_hubImpl->GetInstantEvaluatorFactoryDatabase().GetDuplicateFactoryNames();

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
				const std::map<string, int> duplicateDeferredEvaluatorFactoryNames = g_hubImpl->GetDeferredEvaluatorFactoryDatabase().GetDuplicateFactoryNames();

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
				const GeneratorFactoryDatabase& generatorFactoryDB = g_hubImpl->GetGeneratorFactoryDatabase();

				for (size_t i = 0, n = generatorFactoryDB.GetFactoryCount(); i < n; ++i)
				{
					const client::IGeneratorFactory& generatorFactory = generatorFactoryDB.GetFactory(i);
					const shared::CTypeInfo& itemTypeToGenerate = generatorFactory.GetTypeOfItemsToGenerate();

					if (!g_hubImpl->GetUtils().FindItemFactoryByType(itemTypeToGenerate))
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
				const FunctionFactoryDatabase& functionFactoryDB = g_hubImpl->GetFunctionFactoryDatabase();

				for (size_t i = 0, n = functionFactoryDB.GetFactoryCount(); i < n; ++i)
				{
					const client::IFunctionFactory& functionFactory = functionFactoryDB.GetFactory(i);
					const shared::CTypeInfo& returnTypeOfFunction = functionFactory.GetReturnType();

					if (!g_hubImpl->GetUtils().FindItemFactoryByType(returnTypeOfFunction))
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
					const FunctionFactoryDatabase& functionFactoryDB = g_hubImpl->GetFunctionFactoryDatabase();

					for (size_t i = 0, n = functionFactoryDB.GetFactoryCount(); i < n; ++i)
					{
						const client::IFunctionFactory& functionFactory = functionFactoryDB.GetFactory(i);
						stack_string messagePrefixForPossibleError;
						messagePrefixForPossibleError.Format("Function '%s'", functionFactory.GetName());
						CheckInputParametersConsistency(functionFactory.GetInputParameterRegistry(), messagePrefixForPossibleError.c_str());
					}
				}

				// 2. generator params
				{
					const GeneratorFactoryDatabase& generatorFactoryDB = g_hubImpl->GetGeneratorFactoryDatabase();

					for (size_t i = 0, n = generatorFactoryDB.GetFactoryCount(); i < n; ++i)
					{
						const client::IGeneratorFactory& generatorFactory = generatorFactoryDB.GetFactory(i);
						stack_string messagePrefixForPossibleError;
						messagePrefixForPossibleError.Format("Generator '%s'", generatorFactory.GetName());
						CheckInputParametersConsistency(generatorFactory.GetInputParameterRegistry(), messagePrefixForPossibleError.c_str());
					}
				}

				// 3. instant-evaluator params
				{
					const IInstantEvaluatorFactoryDatabase& ieFactoryDB = g_hubImpl->GetInstantEvaluatorFactoryDatabase();

					for (size_t i = 0, n = ieFactoryDB.GetFactoryCount(); i < n; ++i)
					{
						const client::IInstantEvaluatorFactory& ieFactory = ieFactoryDB.GetFactory(i);
						stack_string messagePrefixForPossibleError;
						messagePrefixForPossibleError.Format("Instant-Evaluator '%s'", ieFactory.GetName());
						CheckInputParametersConsistency(ieFactory.GetInputParameterRegistry(), messagePrefixForPossibleError.c_str());
					}
				}

				// 4. deferred-evaluator params
				{
					const IDeferredEvaluatorFactoryDatabase& deFactoryDB = g_hubImpl->GetDeferredEvaluatorFactoryDatabase();

					for (size_t i = 0, n = deFactoryDB.GetFactoryCount(); i < n; ++i)
					{
						const client::IDeferredEvaluatorFactory& deFactory = deFactoryDB.GetFactory(i);
						stack_string messagePrefixForPossibleError;
						messagePrefixForPossibleError.Format("Deferred-Evaluator '%s'", deFactory.GetName());
						CheckInputParametersConsistency(deFactory.GetInputParameterRegistry(), messagePrefixForPossibleError.c_str());
					}
				}
			}

			// TODO: more consistency checks
		}

		void CStartupConsistencyChecker::CheckInputParametersConsistency(const client::IInputParameterRegistry& registry, const char* errorMessagePrefix)
		{
			for (size_t i = 0, n = registry.GetParameterCount(); i < n; ++i)
			{
				const client::IInputParameterRegistry::SParameterInfo& pi = registry.GetParameter(i);

				// check for known item type
				if (g_hubImpl->GetUtils().FindItemFactoryByType(pi.type) == nullptr)
				{
					string error;
					error.Format("%s: parameter '%s' is of type '%s' but there is no such item-factory registered", errorMessagePrefix, pi.name, pi.type.name());
					m_errors.push_back(error);
				}

				// check for unique name (prevent copy & paste error)
				for (size_t k = i + 1; k < n; ++k)
				{
					const client::IInputParameterRegistry::SParameterInfo& pi2 = registry.GetParameter(k);
					if (strcmp(pi2.name, pi.name) == 0)
					{
						string error;
						error.Format("%s: duplicate parameter name: '%s' (could be a copy & paste error)", errorMessagePrefix, pi2.name);
						m_errors.push_back(error);
					}
				}

				// check for unique memory offset (prevent copy & paste error)
				for (size_t k = i + 1; k < n; ++k)
				{
					const client::IInputParameterRegistry::SParameterInfo& pi2 = registry.GetParameter(k);
					if (pi2.offset == pi.offset)
					{
						string error;
						error.Format("%s: duplicate memory offset for parameter '%s': the parameter '%s' is already using this offset (could be a copy & paste error)", errorMessagePrefix, pi2.name, pi.name);
						m_errors.push_back(error);
					}
				}
			}
		}

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
