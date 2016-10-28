// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// ITextualGlobalRuntimeParamsBlueprint
		//
		//===================================================================================

		struct ITextualGlobalRuntimeParamsBlueprint
		{
			struct SParameterInfo
			{
				explicit                              SParameterInfo(const char* _name, const char* _type, datasource::ISyntaxErrorCollector* _pSyntaxErrorCollector);

				const char*                           name;
				const char*                           type;
				datasource::ISyntaxErrorCollector*    pSyntaxErrorCollector;   // might be nullptr
			};

			virtual                                   ~ITextualGlobalRuntimeParamsBlueprint() {}
			virtual void                              AddParameter(const char* name, const char* type, datasource::SyntaxErrorCollectorUniquePtr pSyntaxErrorCollector) = 0;   // adding a parameter with the same name twice is NOT an error in this class
			virtual size_t                            GetParameterCount() const = 0;
			virtual SParameterInfo                    GetParameter(size_t index) const = 0;
		};

		inline ITextualGlobalRuntimeParamsBlueprint::SParameterInfo::SParameterInfo(const char* _name, const char* _type, datasource::ISyntaxErrorCollector* _pSyntaxErrorCollector)
			: name(_name)
			, type(_type)
			, pSyntaxErrorCollector(_pSyntaxErrorCollector)
		{}

	}
}
