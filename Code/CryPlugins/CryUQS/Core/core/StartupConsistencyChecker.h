// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CStartupConsistencyChecker
		//
		// - should be called once on game startup after all factories have been registered
		//   to see if any obvious problem exists before starting queries
		// - tries to detect issues like missing item-factories that some generators or functions might rely on
		// - runtime issues (like missing/bad runtime-params) can NOT be detected by this facility
		// - also, syntax errors in query-blueprints will not be checked here (it's the responsibility of whoever fills the IQueryBlueprintLibrary to check for such errors)
		//
		//===================================================================================

		class CStartupConsistencyChecker
		{
		public:
			void                    CheckForConsistencyErrors();
			size_t                  GetErrorCount() const;
			const char*             GetError(size_t index) const;

		private:
			void                    CheckInputParametersConsistency(const client::IInputParameterRegistry& registry, const char* errorMessagePrefix);

#if UQS_SCHEMATYC_SUPPORT
			void                    CheckItemConvertersConsistency(const client::IItemConverterCollection& itemConverters, const char* szItemFactoryNameForErrorMessages, std::set<CryGUID>& guidsInUse);
#endif

		private:
			std::vector<string>     m_errors;
		};

	}
}
