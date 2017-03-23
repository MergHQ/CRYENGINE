// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Client
	{
		namespace Internal
		{

			//===================================================================================
			//
			// CGeneratorFactoryBase
			//
			//===================================================================================

			class CGeneratorFactoryBase : public IGeneratorFactory, public IParamsHolderFactory, public CFactoryBase<CGeneratorFactoryBase>
			{
			public:
				// IGeneratorFactory
				virtual const char*                       GetName() const override final;
				virtual const IInputParameterRegistry&    GetInputParameterRegistry() const override final;
				virtual IParamsHolderFactory&             GetParamsHolderFactory() const override final;
				// ~IGeneratorFactory

				// IGeneratorFactory: forward to derived class
				virtual const Shared::CTypeInfo&          GetTypeOfItemsToGenerate() const override = 0;
				virtual GeneratorUniquePtr                CreateGenerator(const void* pParams) override = 0;
				virtual void                              DestroyGenerator(IGenerator* pGeneratorToDestroy) override = 0;
				// ~IGeneratorFactory

				// IParamsHolderFactory: forward to derived class
				virtual ParamsHolderUniquePtr             CreateParamsHolder() override = 0;
				virtual void                              DestroyParamsHolder(IParamsHolder* pParamsHolderToDestroy) override = 0;
				// ~IParamsHolderFactory

			protected:
				explicit                                  CGeneratorFactoryBase(const char* szGeneratorName);

			protected:
				CInputParameterRegistry                   m_inputParameterRegistry;

			private:
				IParamsHolderFactory*                     m_pParamsHolderFactory;      // points to *this; it's a trick to allow GetParamsHolderFactory() return a non-const reference to *this
			};

			inline CGeneratorFactoryBase::CGeneratorFactoryBase(const char* szGeneratorName)
				: CFactoryBase(szGeneratorName)
			{
				m_pParamsHolderFactory = this;
			}

			inline const char* CGeneratorFactoryBase::GetName() const
			{
				return CFactoryBase::GetName();
			}

			inline const IInputParameterRegistry& CGeneratorFactoryBase::GetInputParameterRegistry() const
			{
				return m_inputParameterRegistry;
			}

			inline IParamsHolderFactory& CGeneratorFactoryBase::GetParamsHolderFactory() const
			{
				return *m_pParamsHolderFactory;
			}

		} // namespace Internal

		//===================================================================================
		//
		// CGeneratorFactory<>
		//
		//===================================================================================

		template <class TGenerator>
		class CGeneratorFactory final : public Internal::CGeneratorFactoryBase
		{
		public:
			explicit                                  CGeneratorFactory(const char* szGeneratorName);

			// IGeneratorFactory
			virtual const Shared::CTypeInfo&          GetTypeOfItemsToGenerate() const override;
			virtual GeneratorUniquePtr                CreateGenerator(const void* pParams) override;
			virtual void                              DestroyGenerator(IGenerator* pGeneratorToDestroy) override;
			// ~IGeneratorFactory

			// IParamsHolderFactory
			virtual ParamsHolderUniquePtr             CreateParamsHolder() override;
			virtual void                              DestroyParamsHolder(IParamsHolder* pParamsHolderToDestroy) override;
			// ~IParamsHolderFactory
		};

		template <class TGenerator>
		CGeneratorFactory<TGenerator>::CGeneratorFactory(const char* szGeneratorName)
			: CGeneratorFactoryBase(szGeneratorName)
		{
			typedef typename TGenerator::SParams Params;
			Params::Expose(m_inputParameterRegistry);
		}

		template <class TGenerator>
		const Shared::CTypeInfo& CGeneratorFactory<TGenerator>::GetTypeOfItemsToGenerate() const
		{
			return Shared::SDataTypeHelper<typename TGenerator::ItemType>::GetTypeInfo();
		}

		template <class TGenerator>
		GeneratorUniquePtr CGeneratorFactory<TGenerator>::CreateGenerator(const void* pParams)
		{
			const typename TGenerator::SParams* pActualParams = static_cast<const typename TGenerator::SParams*>(pParams);
#if 0
			TGenerator* pGenerator = new TGenerator(*pActualParams);
#else
			// notice: we assign the instantiated generator to its base class pointer to ensure that the generator type itself (and not accidentally another generator type) was injected at its class definition
			CGeneratorBase<TGenerator, typename TGenerator::ItemType>* pGenerator = new TGenerator(*pActualParams);
#endif
			Internal::CGeneratorDeleter deleter(*this);
			return GeneratorUniquePtr(pGenerator, deleter);
		}

		template <class TGenerator>
		void CGeneratorFactory<TGenerator>::DestroyGenerator(IGenerator* pGeneratorToDestroy)
		{
			delete pGeneratorToDestroy;
		}

		template <class TGenerator>
		ParamsHolderUniquePtr CGeneratorFactory<TGenerator>::CreateParamsHolder()
		{
			Internal::CParamsHolder<typename TGenerator::SParams>* pParamsHolder = new Internal::CParamsHolder<typename TGenerator::SParams>;
			CParamsHolderDeleter deleter(*this);
			return ParamsHolderUniquePtr(pParamsHolder, deleter);
		}

		template <class TGenerator>
		void CGeneratorFactory<TGenerator>::DestroyParamsHolder(IParamsHolder* pParamsHolderToDestroy)
		{
			delete pParamsHolderToDestroy;
		}

	}
}
