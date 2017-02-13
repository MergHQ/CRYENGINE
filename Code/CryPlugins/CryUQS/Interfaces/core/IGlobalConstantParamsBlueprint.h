// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// ITextualGlobalConstantParamsBlueprint
		//
		//===================================================================================

		struct ITextualGlobalConstantParamsBlueprint
		{
			struct SParameterInfo
			{
				explicit                              SParameterInfo(const char* _name, const char* _type, const char* _value, bool _bAddToDebugRenderWorld, datasource::ISyntaxErrorCollector* _pSyntaxErrorCollector);

				const char*                           name;
				const char*                           type;
				const char*                           value;
				bool                                  bAddToDebugRenderWorld;
				datasource::ISyntaxErrorCollector*    pSyntaxErrorCollector;   // might be nullptr
			};

			virtual                                   ~ITextualGlobalConstantParamsBlueprint() {}
			virtual void                              AddParameter(const char* name, const char* type, const char* value, bool bAddToDebugRenderWorld, datasource::SyntaxErrorCollectorUniquePtr pSyntaxErrorCollector) = 0;   // adding a parameter with the same name twice is NOT an error in this class
			virtual size_t                            GetParameterCount() const = 0;
			virtual SParameterInfo                    GetParameter(size_t index) const = 0;
		};

		inline ITextualGlobalConstantParamsBlueprint::SParameterInfo::SParameterInfo(const char* _name, const char* _type, const char* _value, bool _bAddToDebugRenderWorld, datasource::ISyntaxErrorCollector* _pSyntaxErrorCollector)
			: name(_name)
			, type(_type)
			, value(_value)
			, bAddToDebugRenderWorld(_bAddToDebugRenderWorld)
			, pSyntaxErrorCollector(_pSyntaxErrorCollector)
		{}

	}
}
