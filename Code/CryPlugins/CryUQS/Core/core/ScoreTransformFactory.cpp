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

		CScoreTransformFactory::CScoreTransformFactory(const char* szName, const CryGUID& guid, EScoreTransformType scoreTransformType)
			: CFactoryBase(szName, guid)
			, m_scoreTransformType(scoreTransformType)
		{
			// nothing
		}

		const char* CScoreTransformFactory::GetName() const
		{
			return CFactoryBase::GetName();
		}

		const CryGUID& CScoreTransformFactory::GetGUID() const
		{
			return CFactoryBase::GetGUID();
		}

		EScoreTransformType CScoreTransformFactory::GetScoreTransformType() const
		{
			return m_scoreTransformType;
		}

		void CScoreTransformFactory::InstantiateFactories()
		{
			// all currently built-in score transforms

			static const CScoreTransformFactory scoreTransformFactory_linear("Linear", "a4782c62-101b-4039-a68d-c115b65493b1"_uqs_guid, EScoreTransformType::Linear);
			static const CScoreTransformFactory scoreTransformFactory_linearInverse("LinearInverse", "92bb5c1d-f6a3-4477-a24d-adb04b4b6828"_uqs_guid, EScoreTransformType::LinearInverse);
			static const CScoreTransformFactory scoreTransformFactory_sine180("Sine180", "c1593038-fc89-43cc-ac1f-6d2ef563fe14"_uqs_guid, EScoreTransformType::Sine180);
			static const CScoreTransformFactory scoreTransformFactory_sine180inverse("Sine180Inverse", "8b81091c-6de7-45b6-a58c-12d03cbf2dad"_uqs_guid, EScoreTransformType::Sine180Inverse);
			static const CScoreTransformFactory scoreTransformFactory_quadratic("Square", "b52a94b5-3322-4b02-a9d8-c388d6b6cf31"_uqs_guid, EScoreTransformType::Square);
			static const CScoreTransformFactory scoreTransformFactory_quadraticInverse("SquareInverse", "752ad11d-8cf4-42e8-b6ae-8d8fb6ccc7a8"_uqs_guid, EScoreTransformType::SquareInverse);

			s_pDefaultScoreTransformFactory = &scoreTransformFactory_linear;
		}

		const CScoreTransformFactory& CScoreTransformFactory::GetDefaultScoreTransformFactory()
		{
			assert(s_pDefaultScoreTransformFactory);
			return *s_pDefaultScoreTransformFactory;
		}

	}
}
