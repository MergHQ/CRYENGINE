// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CLeafFunctionReturnValue
		//
		//===================================================================================

		class CLeafFunctionReturnValue : public ILeafFunctionReturnValue
		{
		private:

			//===================================================================================
			//
			// UReturnValue
			//
			//===================================================================================

			union UReturnValue
			{
				struct
				{
					client::IItemFactory*                  pItemFactory;
					void*                                  pValue;
				} literal;

				struct
				{
					shared::CUqsString*                    pNameOfGlobalParam;
				} globalParam;

				struct
				{
					// nothing (this struct exists only for consistency purpose)
				} itemIteration;

				struct
				{
					// nothing (this struct exists only for consistency purpose)
				} shuttledItems;
			};

		public:

			                                               CLeafFunctionReturnValue();
			                                               CLeafFunctionReturnValue(const CLeafFunctionReturnValue& rhs);
			                                               ~CLeafFunctionReturnValue();
			CLeafFunctionReturnValue&                      operator=(const CLeafFunctionReturnValue& rhs);

			void                                           SetLiteral(client::IItemFactory& itemFactory, const void* pItemToClone);
			void                                           SetGlobalParam(const char* szNameOfGlobalParam);
			void                                           SetItemIteration();
			void                                           SetShuttledItems();

			bool                                           IsActuallyALeafFunction() const;

			// ILeafFunctionReturnValue
			virtual SLiteralInfo                           GetLiteral(const SQueryBlackboard& blackboard) const override;
			virtual SGlobalParamInfo                       GetGlobalParam(const SQueryBlackboard& blackboard) const override;
			virtual SItemIterationInfo                     GetItemIteration(const SQueryBlackboard& blackboard) const override;
			virtual SShuttledItemsInfo                     GetShuttledItems(const SQueryBlackboard& blackboard) const override;
			// ~ILeafFunctionReturnValue

		private:

			void                                           Clear();
			void                                           CopyOtherToSelf(const CLeafFunctionReturnValue& other);

		private:
			
			client::IFunctionFactory::ELeafFunctionKind    m_leafFunctionKind;
			UReturnValue                                   m_returnValue;
		};

	}
}

