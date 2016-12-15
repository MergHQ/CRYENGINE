// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client { struct IFunctionFactory; }
	namespace client { struct IInputParameterRegistry; }

	namespace core
	{

		class CInputBlueprint;

		//===================================================================================
		//
		// CTextualInputBlueprint
		//
		// - used by a loader to build a preliminary input hierarchy for a blueprint from an abstract data source (e. g. XML, JSON, UI)
		// - that input hierarchy cannot be used directly, it needs to get validated and resolved into a CInputBlueprint
		//
		//===================================================================================

		class CTextualInputBlueprint : public ITextualInputBlueprint
		{
		public:
			explicit                                    CTextualInputBlueprint();
			                                            ~CTextualInputBlueprint();

			virtual const char*                         GetParamName() const override;
			virtual const char*                         GetFuncName() const override;
			virtual const char*                         GetFuncReturnValueLiteral() const override;
			virtual const char*                         GetAddReturnValueToDebugRenderWorldUponExecution() const override;

			virtual void                                SetParamName(const char* szParamName) override;
			virtual void                                SetFuncName(const char* szFuncName) override;
			virtual void                                SetFuncReturnValueLiteral(const char* szValue) override;
			virtual void                                SetAddReturnValueToDebugRenderWorldUponExecution(const char* szAddReturnValueToDebugRenderWorldUponExecution) override;

			virtual ITextualInputBlueprint&             AddChild(const char* paramName, const char* funcName, const char* funcReturnValueLiteral, const char* addReturnValueToDebugRenderWorldUponExecution) override;
			virtual size_t                              GetChildCount() const override;
			virtual const ITextualInputBlueprint&       GetChild(size_t index) const override;
			virtual const ITextualInputBlueprint*       FindChildByParamName(const char* paramName) const override;

			virtual void                                SetSyntaxErrorCollector(datasource::SyntaxErrorCollectorUniquePtr ptr) override;
			virtual datasource::ISyntaxErrorCollector*  GetSyntaxErrorCollector() const override;

		private:
			explicit                                    CTextualInputBlueprint(const char* paramName, const char* funcName, const char* funcReturnValueLiteral, const char* addReturnValueToDebugRenderWorldUponExecution);

			                                            UQS_NON_COPYABLE(CTextualInputBlueprint);

		private:
			string                                      m_paramName;
			string                                      m_funcName;
			string                                      m_funcReturnValueLiteral;
			string                                      m_addReturnValueToDebugRenderWorldUponExecution;
			std::vector<CTextualInputBlueprint*>        m_children;
			datasource::SyntaxErrorCollectorUniquePtr   m_pSyntaxErrorCollector;
		};

		class CQueryBlueprint;

		//===================================================================================
		//
		// CInputBlueprint
		//
		// - result after resolving the preliminary input hierarchy that was loaded from an abstract data source (e. g. XML, JSON, UI)
		// - allows traversing the hierarchy and grabbing the function factories for building the actual function blueprints
		//
		//===================================================================================

		class CInputBlueprint
		{
		public:
			explicit                            CInputBlueprint();
			                                    ~CInputBlueprint();

			bool                                Resolve(const ITextualInputBlueprint& sourceParent, const client::IInputParameterRegistry& inputParamsReg, const CQueryBlueprint& queryBlueprintForGlobalParamChecking, bool bResolvingForAGenerator);

			size_t                              GetChildCount() const;
			const CInputBlueprint&              GetChild(size_t index) const;
			client::IFunctionFactory*           GetFunctionFactory() const;
			const char*                         GetFunctionReturnValueLiteral() const;
			bool                                GetAddReturnValueToDebugRenderWorldUponExecution() const;

		private:
			explicit                            CInputBlueprint(client::IFunctionFactory& functionFactory, const char* functionReturnValueLiteral, bool bAddReturnValueToDebugRenderWorldUponExecution);

			                                    UQS_NON_COPYABLE(CInputBlueprint);

		private:
			client::IFunctionFactory*           m_pFunctionFactory;
			string                              m_functionReturnValueLiteral;
			bool                                m_bAddReturnValueToDebugRenderWorldUponExecution;
			std::vector<CInputBlueprint*>       m_children;	// in order of how the input parameters need to appear at runtime
		};

	}
}
