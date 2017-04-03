#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Core
	{

		//===================================================================================
		//
		// CScoreTransformFactory
		//
		//===================================================================================

		const CScoreTransformFactory* CScoreTransformFactory::s_pDefaultScoreTransformFactory;

		CScoreTransformFactory::CScoreTransformFactory(const char* szName, EScoreTransformType scoreTransformType)
			: CFactoryBase(szName)
			, m_scoreTransformType(scoreTransformType)
		{
			// nothing
		}

		const char* CScoreTransformFactory::GetName() const
		{
			return CFactoryBase::GetName();
		}

		EScoreTransformType CScoreTransformFactory::GetScoreTransformType() const
		{
			return m_scoreTransformType;
		}

		void CScoreTransformFactory::InstantiateFactories()
		{
			// all currently built-in score transforms

			static const CScoreTransformFactory scoreTransformFactory_linear("Linear", EScoreTransformType::Linear);
			static const CScoreTransformFactory scoreTransformFactory_linearInverse("LinearInverse", EScoreTransformType::LinearInverse);
			static const CScoreTransformFactory scoreTransformFactory_sine180("Sine180", EScoreTransformType::Sine180);
			static const CScoreTransformFactory scoreTransformFactory_sine180inverse("Sine180Inverse", EScoreTransformType::Sine180Inverse);
			static const CScoreTransformFactory scoreTransformFactory_quadratic("Square", EScoreTransformType::Square);
			static const CScoreTransformFactory scoreTransformFactory_quadraticInverse("SquareInverse", EScoreTransformType::SquareInverse);

			s_pDefaultScoreTransformFactory = &scoreTransformFactory_linear;
		}

		const CScoreTransformFactory& CScoreTransformFactory::GetDefaultScoreTransformFactory()
		{
			assert(s_pDefaultScoreTransformFactory);
			return *s_pDefaultScoreTransformFactory;
		}

	}
}
