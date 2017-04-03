// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Core
	{

		//===================================================================================
		//
		// ITextualInstantEvaluatorBlueprint
		//
		//===================================================================================

		struct ITextualInstantEvaluatorBlueprint
		{
			virtual                                       ~ITextualInstantEvaluatorBlueprint() {}
			virtual void                                  SetEvaluatorName(const char* szEvaluatorName) = 0;
			virtual void                                  SetWeight(float weight) = 0;
			virtual float                                 GetWeight() const = 0;
			virtual ITextualInputBlueprint&               GetInputRoot() = 0;
			virtual const char*                           GetEvaluatorName() const = 0;
			virtual const ITextualInputBlueprint&         GetInputRoot() const = 0;
			virtual void                                  SetScoreTransform(const char* szScoreTransform) = 0;
			virtual const char*                           GetScoreTransform() const = 0;
			virtual void                                  SetNegateDiscard(bool bNegateDiscard) = 0;
			virtual bool                                  GetNegateDiscard() const = 0;
			virtual void                                  SetSyntaxErrorCollector(DataSource::SyntaxErrorCollectorUniquePtr pSyntaxErrorCollector) = 0;
			virtual DataSource::ISyntaxErrorCollector*    GetSyntaxErrorCollector() const = 0;
		};

	}
}
