// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Core
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
				explicit                              SParameterInfo(const char* _szName, const char* _szType, bool _bAddToDebugRenderWorld, DataSource::ISyntaxErrorCollector* _pSyntaxErrorCollector);

				const char*                           szName;
				const char*                           szType;
				bool                                  bAddToDebugRenderWorld;  // if true and the underlying item type supports debug-rendering, than this will add the parameter's visual representation to the IDebugRenderWorldPersistent
				DataSource::ISyntaxErrorCollector*    pSyntaxErrorCollector;   // might be nullptr
			};

			virtual                                   ~ITextualGlobalRuntimeParamsBlueprint() {}
			virtual void                              AddParameter(const char* szName, const char* szType, bool bAddToDebugRenderWorld, DataSource::SyntaxErrorCollectorUniquePtr pSyntaxErrorCollector) = 0;   // adding a parameter with the same name twice is NOT an error in this class
			virtual size_t                            GetParameterCount() const = 0;
			virtual SParameterInfo                    GetParameter(size_t index) const = 0;
		};

		inline ITextualGlobalRuntimeParamsBlueprint::SParameterInfo::SParameterInfo(const char* _szName, const char* _szType, bool _bAddToDebugRenderWorld, DataSource::ISyntaxErrorCollector* _pSyntaxErrorCollector)
			: szName(_szName)
			, szType(_szType)
			, bAddToDebugRenderWorld(_bAddToDebugRenderWorld)
			, pSyntaxErrorCollector(_pSyntaxErrorCollector)
		{}

	}
}
