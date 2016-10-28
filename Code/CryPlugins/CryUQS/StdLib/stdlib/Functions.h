// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace stdlib
	{

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
