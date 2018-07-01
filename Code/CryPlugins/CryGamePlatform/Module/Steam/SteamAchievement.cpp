// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "SteamAchievement.h"
#include "SteamStatistics.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Steam
		{
			CAchievement::CAchievement(CStatistics& steamStats, const char* name, bool bAchieved)
				: m_stats(steamStats)
				, m_name(name)
				, m_bAchieved(bAchieved)
			{

			}

			bool CAchievement::Reset()
			{
				ISteamUserStats* pSteamUserStats = SteamUserStats();
				if (!pSteamUserStats)
				{
					return false;
				}
				return pSteamUserStats->ClearAchievement(m_name);
			}

			bool CAchievement::Achieve()
			{
				ISteamUserStats* pSteamUserStats = SteamUserStats();
				if (!pSteamUserStats)
				{
					return false;
				}
				bool bResult = pSteamUserStats->SetAchievement(m_name);

				m_stats.Upload();

				return bResult;
			}
		}
	}
}