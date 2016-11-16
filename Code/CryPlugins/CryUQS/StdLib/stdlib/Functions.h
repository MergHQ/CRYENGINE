// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace stdlib
	{

		//===================================================================================
		//
		// CFunction_Vec3Add
		//
		// - adds two Vec3's and returns the sum
		//
		//===================================================================================

		class CFunction_Vec3Add : public client::CFunctionBase<CFunction_Vec3Add, Vec3, client::IFunctionFactory::ELeafFunctionKind::None>
		{
		public:
			struct SParams
			{
				Vec3      v1;
				Vec3      v2;

				UQS_EXPOSE_PARAMS_BEGIN
					UQS_EXPOSE_PARAM("v1", v1);
					UQS_EXPOSE_PARAM("v2", v2);
				UQS_EXPOSE_PARAMS_END
			};

		public:
			explicit   CFunction_Vec3Add(const SCtorContext& ctorContext);
			Vec3       DoExecute(const SExecuteContext& executeContext, const SParams& params) const;
		};

		//===================================================================================
		//
		// CFunction_PosFromEntity
		//
		// - returns the position of a given entity
		// - causes an exception if no such entity exists
		//
		//===================================================================================

		class CFunction_PosFromEntity : public client::CFunctionBase<CFunction_PosFromEntity, Vec3, client::IFunctionFactory::ELeafFunctionKind::None>
		{
		public:
			struct SParams
			{
				EntityIdWrapper entityId;

				UQS_EXPOSE_PARAMS_BEGIN
					UQS_EXPOSE_PARAM("entityId", entityId);
				UQS_EXPOSE_PARAMS_END
			};

		public:
			explicit   CFunction_PosFromEntity(const SCtorContext& ctorContext);
			Vec3       DoExecute(const SExecuteContext& executeContext, const SParams& params) const;
		};

	}
}
