// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if UQS_SCHEMATYC_SUPPORT

namespace uqs
{
	namespace core
	{

		class CSchematycUqsComponent; // below


		//===================================================================================
		//
		// CSchematycEnvDataTypeFinder
		//
		//===================================================================================

		class CSchematycEnvDataTypeFinder
		{
		public:

			static const Schematyc::IEnvDataType*   FindEnvDataTypeByTypeName(const Schematyc::CTypeName& typeNameToSearchFor);

		private:

			explicit                                CSchematycEnvDataTypeFinder(const Schematyc::CTypeName& typeNameToSearchFor);
			Schematyc::EVisitStatus                 OnVisitEnvDataType(const Schematyc::IEnvDataType& dataType);

		private:

			const Schematyc::CTypeName&             m_typeNameToSearchFor;
			const Schematyc::IEnvDataType*          m_pFoundDataType;
		};

		//===================================================================================
		//
		// CSchematycUqsComponentEnvFunctionBase
		//
		// - abstract base class for a schematyc function that runs on the CSchematycUqsComponent
		// - derived classes have to implement the ExecuteOnSchematycUqsComponent() method
		//
		//===================================================================================

		class CSchematycUqsComponentEnvFunctionBase : public Schematyc::CEnvElementBase<Schematyc::IEnvFunction>
		{
		private:

			struct SMyParamBinding
			{
				SMyParamBinding()
					: idx(Schematyc::InvalidIdx)
					, id(Schematyc::InvalidId)
				{}

				Schematyc::EnvFunctionUtils::ParamFlags   flags;
				uint32                                    idx;
				uint32                                    id;
				string                                    name;
				Schematyc::CAnyValuePtr                   pData;
			};

			struct SMyFunctionBinding
			{
				SMyFunctionBinding()
					: totalParamsCount(0)
					, inputCount(0)
					, outputCount(0)
				{}

				SMyParamBinding                           params[Schematyc::EnvFunctionUtils::MaxParams];
				uint32                                    totalParamsCount;
				uint32                                    inputCount;
				uint32                                    inputs[Schematyc::EnvFunctionUtils::MaxParams];
				uint32                                    outputCount;
				uint32                                    outputs[Schematyc::EnvFunctionUtils::MaxParams];
			};

		public:

			explicit                                      CSchematycUqsComponentEnvFunctionBase(const Schematyc::SSourceFileInfo& sourceFileInfo);
			virtual                                       ~CSchematycUqsComponentEnvFunctionBase();

			// IEnvElement
			virtual bool                                  IsValidScope(Schematyc::IEnvElement& scope) const override final;
			// ~IEnvElement

			// IEnvFunction
			virtual bool                                  Validate() const override final;
			virtual Schematyc::EnvFunctionFlags           GetFunctionFlags() const override final;
			virtual const Schematyc::CCommonTypeDesc*     GetObjectTypeDesc() const override final;
			virtual uint32                                GetInputCount() const override final;
			virtual uint32                                GetInputId(uint32 inputIdx) const override final;
			virtual const char*                           GetInputName(uint32 inputIdx) const override final;
			virtual const char*                           GetInputDescription(uint32 inputIdx) const override final;
			virtual Schematyc::CAnyConstPtr               GetInputData(uint32 inputIdx) const override final;
			virtual uint32                                GetOutputCount() const override final;
			virtual uint32                                GetOutputId(uint32 outputIdx) const override final;
			virtual const char*                           GetOutputName(uint32 outputIdx) const override final;
			virtual const char*                           GetOutputDescription(uint32 outputIdx) const override final;
			virtual Schematyc::CAnyConstPtr               GetOutputData(uint32 outputIdx) const override final;
			virtual void                                  Execute(Schematyc::CRuntimeParamMap& params, void* pObject) const override final;
			// ~IEnvFunction

			bool                                          AreAllParamtersRecognized() const;

		protected:

			void                                          AddInputParamByTypeName(const char* szParamName, uint32 id, const Schematyc::CTypeName& typeName);
			void                                          AddInputParamByTypeDesc(const char* szParamName, uint32 id, const Schematyc::CCommonTypeDesc& typeDesc);
			void                                          AddOutputParamByTypeName(const char* szParamName, uint32 id, const Schematyc::CTypeName& typeName);
			void                                          AddOutputParamByTypeDesc(const char* szParamName, uint32 id, const Schematyc::CCommonTypeDesc& typeDesc);
			Schematyc::CAnyConstRef                       GetUntypedInputParam(const Schematyc::CRuntimeParamMap& params, uint32 inputIdx) const;
			Schematyc::CAnyRef                            GetUntypedOutputParam(Schematyc::CRuntimeParamMap& params, uint32 outputIdx) const;
			template <class T> const T&                   GetTypedInputParam(const Schematyc::CRuntimeParamMap& params, uint32 inputIdx) const;
			template <class T> T&                         GetTypedOutputParam(Schematyc::CRuntimeParamMap& params, uint32 outputIdx) const;

			// - replaces "::" by ": " in given name
			// - this is required since Schematyc seems to interpret "::" as some kind of namespace and then messes things up in the UI
			static stack_string                           PatchName(const char* szName);

		private:

			virtual void                                  ExecuteOnSchematycUqsComponent(Schematyc::CRuntimeParamMap& params, CSchematycUqsComponent& schematycUqsComponent) const = 0;

			SMyParamBinding                               CreateParam(const char* szParamName, uint32 id, const Schematyc::CTypeName* pTypeName, const Schematyc::CCommonTypeDesc* pTypeDesc, Schematyc::EnvFunctionUtils::EParamFlags paramFlags) const;
			void                                          AddInputParam(const char* szParamName, uint32 id, const Schematyc::CTypeName* pTypeName, const Schematyc::CCommonTypeDesc* pTypeDesc);
			void                                          AddOutputParam(const char* szParamName, uint32 id, const Schematyc::CTypeName* pTypeName, const Schematyc::CCommonTypeDesc* pTypeDesc);

		private:

			SMyFunctionBinding                            m_functionBinding;

		};

		template <class T>
		inline const T& CSchematycUqsComponentEnvFunctionBase::GetTypedInputParam(const Schematyc::CRuntimeParamMap& params, uint32 inputIdx) const
		{
			Schematyc::CAnyConstRef ref = GetUntypedInputParam(params, inputIdx);
			assert(ref.GetTypeDesc().GetName() == Schematyc::GetTypeName<T>());
			return *static_cast<const T*>(ref.GetValue());
		}

		template <class T>
		inline T& CSchematycUqsComponentEnvFunctionBase::GetTypedOutputParam(Schematyc::CRuntimeParamMap& params, uint32 outputIdx) const
		{
			Schematyc::CAnyRef ref = GetUntypedOutputParam(params, outputIdx);
			assert(ref.GetTypeDesc().GetName() == Schematyc::GetTypeName<T>());
			return *static_cast<T*>(ref.GetValue());
		}

		//===================================================================================
		//
		// CSchematycUqsComponentEnvFunction_AddParam
		//
		//===================================================================================

		class CSchematycUqsComponentEnvFunction_AddParam : public CSchematycUqsComponentEnvFunctionBase
		{
		public:

			static void                    GenerateAddParamFunctionsInRegistrationScope(Schematyc::CEnvRegistrationScope& scope, client::IItemFactory& itemFactory);

		private:

			explicit                       CSchematycUqsComponentEnvFunction_AddParam(const Schematyc::SSourceFileInfo& sourceFileInfo, client::IItemFactory& itemFactory, const client::IItemConverter* pOptionalFromForeignTypeConverter);

			// CSchematycUqsComponentEnvFunctionBase
			virtual void                   ExecuteOnSchematycUqsComponent(Schematyc::CRuntimeParamMap& params, CSchematycUqsComponent& schematycEntityUqsComponent) const override;
			// ~CSchematycUqsComponentEnvFunctionBase

		private:

			client::IItemFactory&          m_itemFactory;
			const client::IItemConverter*  m_pFromForeignTypeConverter;
		};

		//===================================================================================
		//
		// CSchematycUqsComponentEnvFunction_GetItemFromResultSet
		//
		//===================================================================================

		class CSchematycUqsComponentEnvFunction_GetItemFromResultSet : public CSchematycUqsComponentEnvFunctionBase
		{
		public:

			static void                    GenerateGetItemFromResultSetFunctionsInRegistrationScope(Schematyc::CEnvRegistrationScope& scope, client::IItemFactory& itemFactory);

		private:

			explicit                       CSchematycUqsComponentEnvFunction_GetItemFromResultSet(const Schematyc::SSourceFileInfo& sourceFileInfo, client::IItemFactory& itemFactory, const client::IItemConverter* pOptionalToForeignTypeConverter);

			// CSchematycUqsComponentEnvFunctionBase
			virtual void                   ExecuteOnSchematycUqsComponent(Schematyc::CRuntimeParamMap& params, CSchematycUqsComponent& schematycEntityUqsComponent) const override;
			// ~CSchematycUqsComponentEnvFunctionBase

		private:

			client::IItemFactory&          m_itemFactory;
			const client::IItemConverter*  m_pToForeignTypeConverter;
		};

		//===================================================================================
		//
		// CSchematycUqsComponent
		//
		//===================================================================================

		class CSchematycUqsComponent final : public Schematyc::CComponent
		{
		public:

			//===================================================================================
			//
			// SQueryIdWrapper - wrapper type needed, because default ctor of CQueryID is hidden
			//
			//===================================================================================

			struct SQueryIdWrapper
			{
				                                                SQueryIdWrapper();
				explicit                                        SQueryIdWrapper(const CQueryID& id);
				bool                                            operator==(const SQueryIdWrapper& other) const;
				bool                                            Serialize(Serialization::IArchive& archive, const char* szName, const char* szLabel);
				static void                                     ReflectType(Schematyc::CTypeDesc<SQueryIdWrapper>& desc);
				static void                                     ToString(Schematyc::IString& output, const SQueryIdWrapper& input);
				static void                                     Register(Schematyc::CEnvRegistrationScope& scope);
				static bool                                     SchematycFunction_Equal(const SQueryIdWrapper& queryId1, const SQueryIdWrapper& queryId2);

				CQueryID                                        queryId;
			};

		private:

			//===================================================================================
			//
			// SQueryFinishedSignal - sent when a query finishes without having run into an exception
			//
			//===================================================================================

			struct SQueryFinishedSignal
			{
				                                                SQueryFinishedSignal();
				explicit                                        SQueryFinishedSignal(const SQueryIdWrapper& _queryId, int32 _resultCount);
				static void                                     ReflectType(Schematyc::CTypeDesc<SQueryFinishedSignal>& desc);

				SQueryIdWrapper                                 queryId;
				int32                                           resultCount;
			};

			//===================================================================================
			//
			// SQueryExceptionSignal - sent when a query runs into an exception (thus having been stopped prematurely and not having been able to come up with a result set)
			//
			//===================================================================================

			struct SQueryExceptionSignal
			{
				                                                SQueryExceptionSignal();
				explicit                                        SQueryExceptionSignal(const SQueryIdWrapper& _queryId, const Schematyc::CSharedString& _exceptionMessage);
				static void                                     ReflectType(Schematyc::CTypeDesc<SQueryExceptionSignal>& desc);

				SQueryIdWrapper                                 queryId;
				Schematyc::CSharedString                        exceptionMessage;
			};

			//===================================================================================
			//
			// SUpcomingQueryInfo
			//
			//===================================================================================

			struct SUpcomingQueryInfo
			{
				void                                            Clear();

				CQueryBlueprintID                               queryBlueprintID;
				Schematyc::CSharedString                        querierName;
				shared::CVariantDict                            runtimeParams;
			};

			//===================================================================================
			//
			// SQueryResultSetUniquePtrWrapper - helper to make std::unique_ptr work as values inside a std::map
			//
			//===================================================================================

			struct SQueryResultSetUniquePtrWrapper
			{
				SQueryResultSetUniquePtrWrapper()
				{}

				SQueryResultSetUniquePtrWrapper(const SQueryResultSetUniquePtrWrapper&)
				{}

				QueryResultSetUniquePtr                         pQueryResultSet;
			};

		public:

			explicit                                            CSchematycUqsComponent();
			                                                    ~CSchematycUqsComponent();

			// Schematyc::CComponent
			virtual void                                        Run(Schematyc::ESimulationMode simulationMode) override;
			virtual void                                        Shutdown() override;
			// ~Schematyc::CComponent

			// these 2 are called by the generated "AddParam" and "GetItemFromResultSet" functions
			shared::CVariantDict&                               GetRuntimeParamsStorageOfUpcomingQuery();
			const IQueryResultSet*                              GetQueryResultSet(const CQueryID& queryID) const;

			static void                                         ReflectType(Schematyc::CTypeDesc<CSchematycUqsComponent>& desc);
			static void                                         Register(Schematyc::IEnvRegistrar& registrar);

		private:

			void                                                SchematycFunction_BeginQuery(const Schematyc::CSharedString& queryBlueprintName, const Schematyc::CSharedString& querierName);
			void                                                SchematycFunction_StartQuery(SQueryIdWrapper& outQueryId);
			void                                                Update(const Schematyc::SUpdateContext& updateContext);
			void                                                CancelRunningQueriesAndClearFinishedOnes();
			void                                                FlushFinishedQueries();
			void                                                OnQueryResult(const SQueryResult& queryResult);

		private:

			Schematyc::CConnectionScope                         m_connectionScope;
			SUpcomingQueryInfo                                  m_upcomingQueryInfo;
			std::set<CQueryID>                                  m_runningQueries;
			std::map<CQueryID, SQueryResultSetUniquePtrWrapper> m_succeededQueries;
			std::map<CQueryID, string>                          m_failedQueries;       // string = exception message
		};

	}
}

#endif // UQS_SCHEMATYC_SUPPORT
