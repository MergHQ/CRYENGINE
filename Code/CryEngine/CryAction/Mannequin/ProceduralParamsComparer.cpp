// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryExtension/ClassWeaver.h>
#include <ICryMannequin.h>
#include <Mannequin/Serialization.h>
#include <CrySerialization/IArchiveHost.h>

class CProceduralParamsComparerDefault
	: public IProceduralParamsComparer
{
public:
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IProceduralParamsComparer)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralParamsComparerDefault, IProceduralParamsComparerDefaultName, 0xfc53bd9248534faa, 0xab0fb42b24e55b3e)

	virtual ~CProceduralParamsComparerDefault() {}

	virtual bool Equal(const IProceduralParams& lhs, const IProceduralParams& rhs) const override
	{
		return Serialization::CompareBinary(lhs, rhs);
	}
};

CRYREGISTER_CLASS(CProceduralParamsComparerDefault);
