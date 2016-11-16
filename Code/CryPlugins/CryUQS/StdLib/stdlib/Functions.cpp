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
			static const client::CFunctionFactory<CFunction_Vec3Add> functionFactory_Vec3Add("std::Vec3Add");
			static const client::CFunctionFactory<CFunction_PosFromEntity> functionFactory_PosFromEntity("std::PosFromEntity");
		}

		//===================================================================================
		//
		// CFunction_Vec3Add
		//
		//===================================================================================

		CFunction_Vec3Add::CFunction_Vec3Add(const SCtorContext& ctorContext)
			: CFunctionBase(ctorContext)
		{
			// nothing
		}

		Vec3 CFunction_Vec3Add::DoExecute(const SExecuteContext& executeContext, const SParams& params) const
		{
			return params.v1 + params.v2;
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

		Vec3 CFunction_PosFromEntity::DoExecute(const SExecuteContext& executeContext, const SParams& params) const
		{
			if (const IEntity* pEntity = gEnv->pEntitySystem->GetEntity(params.entityId.value))
			{
				return pEntity->GetPos();
			}
			else
			{
				executeContext.error.Format("could not find entity with entityId = %i", static_cast<int>(params.entityId.value));
				executeContext.bExceptionOccurred = true;
				return Vec3Constants<float>::fVec3_Zero;
			}
		}

	}
}
