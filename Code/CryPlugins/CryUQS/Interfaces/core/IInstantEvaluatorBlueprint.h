// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// ITextualInstantEvaluatorBlueprint
		//
		//===================================================================================

		struct ITextualInstantEvaluatorBlueprint
		{
			virtual                                       ~ITextualInstantEvaluatorBlueprint() {}
			virtual void                                  SetEvaluatorName(const char* evaluatorName) = 0;
			virtual void                                  SetWeight(float weight) = 0;
			virtual float                                 GetWeight() const = 0;
			virtual ITextualInputBlueprint&               GetInputRoot() = 0;
			virtual const char*                           GetEvaluatorName() const = 0;
			virtual const ITextualInputBlueprint&         GetInputRoot() const = 0;
			virtual void                                  SetSyntaxErrorCollector(datasource::SyntaxErrorCollectorUniquePtr ptr) = 0;
			virtual datasource::ISyntaxErrorCollector*    GetSyntaxErrorCollector() const = 0;
		};

	}
}
