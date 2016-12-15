// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client
	{
		namespace internal
		{
			class CDeferredEvaluatorDeleter;    // below
		}

		//===================================================================================
		//
		// DeferredEvaluatorUniquePtr
		//
		//===================================================================================

		typedef std::unique_ptr<IDeferredEvaluator, internal::CDeferredEvaluatorDeleter>  DeferredEvaluatorUniquePtr;

		//===================================================================================
		//
		// IDeferredEvaluatorFactory
		//
		//===================================================================================

		struct IDeferredEvaluatorFactory
		{
			virtual                                    ~IDeferredEvaluatorFactory() {}
			virtual const char*                        GetName() const = 0;
			virtual const IInputParameterRegistry&     GetInputParameterRegistry() const = 0;
			virtual DeferredEvaluatorUniquePtr         CreateDeferredEvaluator(const void* pParams) = 0;
			virtual void                               DestroyDeferredEvaluator(IDeferredEvaluator* pDeferredEvaluatorToDestroy) = 0;
			virtual IParamsHolderFactory&              GetParamsHolderFactory() const = 0;
		};

		namespace internal
		{

			//===================================================================================
			//
			// CDeferredEvaluatorDeleter
			//
			//===================================================================================

			class CDeferredEvaluatorDeleter
			{
			public:
				explicit                                CDeferredEvaluatorDeleter();         // default ctor is required for when smart pointer using this deleter gets implicitly constructed via nullptr (i. e. with only 1 argument for the smart pointer's ctor)
				explicit                                CDeferredEvaluatorDeleter(IDeferredEvaluatorFactory& deferredEvaluatorFactory);
				void                                    operator()(IDeferredEvaluator* deferredEvaluatorToDelete);

			private:
				IDeferredEvaluatorFactory*              m_deferredEvaluatorFactory;          // this one created the deferred-evaluator before
			};

			inline CDeferredEvaluatorDeleter::CDeferredEvaluatorDeleter()
				: m_deferredEvaluatorFactory(nullptr)
			{}

			inline CDeferredEvaluatorDeleter::CDeferredEvaluatorDeleter(IDeferredEvaluatorFactory& deferredEvaluatorFactory)
				: m_deferredEvaluatorFactory(&deferredEvaluatorFactory)
			{}

			inline void CDeferredEvaluatorDeleter::operator()(IDeferredEvaluator* deferredEvaluatorToDelete)
			{
				assert(m_deferredEvaluatorFactory);
				m_deferredEvaluatorFactory->DestroyDeferredEvaluator(deferredEvaluatorToDelete);
			}

		} // namespace internal

	}
}
