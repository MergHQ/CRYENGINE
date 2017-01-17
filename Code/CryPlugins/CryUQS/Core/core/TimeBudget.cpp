// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		void CTimeBudget::Restart(const CTimeValue& amountOfTimeFromNowOn)
		{
			m_futureTimestampOfExhaustion = gEnv->pTimer->GetAsyncTime() + amountOfTimeFromNowOn;
		}

		bool CTimeBudget::IsExhausted() const
		{
			return (gEnv->pTimer->GetAsyncTime() >= m_futureTimestampOfExhaustion);
		}

	}
}
