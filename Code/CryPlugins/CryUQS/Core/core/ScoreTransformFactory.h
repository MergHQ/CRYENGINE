// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

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
		
		class CScoreTransformFactory : public IScoreTransformFactory, public Shared::CFactoryBase<CScoreTransformFactory>
		{
		public:

			explicit                              CScoreTransformFactory(const char* szName, EScoreTransformType scoreTransformType);

			// IScoreTransformFactory
			virtual const char*                   GetName() const override;
			virtual EScoreTransformType           GetScoreTransformType() const override;
			// ~IScoreTransformFactory

			static void                           InstantiateFactories();
			static const CScoreTransformFactory&  GetDefaultScoreTransformFactory();   // this may only be called after InstantiateFactories(); will assert() and crash otherwise

		private:

			EScoreTransformType                   m_scoreTransformType;
			static const CScoreTransformFactory*  s_pDefaultScoreTransformFactory;     // will be set by InstantiateFactories()
		};

		//===================================================================================
		//
		// ScoreTransformFactoryDatabase
		//
		//===================================================================================

		typedef CFactoryDatabase<IScoreTransformFactory> ScoreTransformFactoryDatabase;

	}
}
