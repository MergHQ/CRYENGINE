// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace stdlib
	{

		//===================================================================================
		//
		// CFunction_Pos3AddOfs3
		//
		// - adds an Ofs3 to a Pos3 and returns the new Pos3
		//
		//===================================================================================

		class CFunction_Pos3AddOfs3 : public client::CFunctionBase<CFunction_Pos3AddOfs3, Pos3, client::IFunctionFactory::ELeafFunctionKind::None>
		{
		public:
			struct SParams
			{
				Pos3      pos;
				Ofs3      ofs;

				UQS_EXPOSE_PARAMS_BEGIN
					UQS_EXPOSE_PARAM("pos", pos);
					UQS_EXPOSE_PARAM("ofs", ofs);
				UQS_EXPOSE_PARAMS_END
			};

		public:
			explicit   CFunction_Pos3AddOfs3(const SCtorContext& ctorContext);
			Pos3       DoExecute(const SExecuteContext& executeContext, const SParams& params) const;
		};

		//===================================================================================
		//
		// CFunction_PosFromEntity
		//
		// - returns the position of a given entity
		// - causes an exception if no such entity exists
		//
		//===================================================================================

		class CFunction_PosFromEntity : public client::CFunctionBase<CFunction_PosFromEntity, Pos3, client::IFunctionFactory::ELeafFunctionKind::None>
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
			Pos3       DoExecute(const SExecuteContext& executeContext, const SParams& params) const;
		};

	}
}
