// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryEntitySystem/IEntitySystem.h>

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace stdlib
	{

		void CStdLibRegistration::InstantiateFunctionFactoriesForRegistration()
		{
			static const client::CFunctionFactory<CFunction_Pos3AddOfs3> functionFactory_Vec3Add("std::Pos3AddOfs3");
			static const client::CFunctionFactory<CFunction_PosFromEntity> functionFactory_PosFromEntity("std::PosFromEntity");
		}

		//===================================================================================
		//
		// CFunction_Pos3AddOfs3
		//
		//===================================================================================

		CFunction_Pos3AddOfs3::CFunction_Pos3AddOfs3(const SCtorContext& ctorContext)
			: CFunctionBase(ctorContext)
		{
			// nothing
		}

		Pos3 CFunction_Pos3AddOfs3::DoExecute(const SExecuteContext& executeContext, const SParams& params) const
		{
			return Pos3(params.pos.value + params.ofs.value);
		}

		//===================================================================================
		//
		// CFunction_PosFromEntity
		//
		//===================================================================================

		CFunction_PosFromEntity::CFunction_PosFromEntity(const SCtorContext& ctorContext)
			: CFunctionBase(ctorContext)
		{
			// nothing
		}

		Pos3 CFunction_PosFromEntity::DoExecute(const SExecuteContext& executeContext, const SParams& params) const
		{
			if (const IEntity* pEntity = gEnv->pEntitySystem->GetEntity(params.entityId.value))
			{
				return Pos3(pEntity->GetPos());
			}
			else
			{
				executeContext.error.Format("could not find entity with entityId = %i", static_cast<int>(params.entityId.value));
				executeContext.bExceptionOccurred = true;
				return Pos3(Vec3Constants<float>::fVec3_Zero);
			}
		}

	}
}
