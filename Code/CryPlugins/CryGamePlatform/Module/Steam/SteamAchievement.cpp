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
			CAchievement::CAchievement(CStatistics& steamStats, const char* name, bool achieved)
				: m_stats(steamStats)
				, m_name(name)
				, m_achieved(achieved)
			{

			}

			bool CAchievement::Reset()
			{
				bool cleared = false;

				if (m_achieved)
				{
					if (ISteamUserStats* pSteamUserStats = SteamUserStats())
					{
						cleared = pSteamUserStats->ClearAchievement(m_name);
						if (cleared)
						{
							m_achieved = false;
							m_stats.Upload();
						}
					}
				}

				return cleared;
			}

			bool CAchievement::Achieve()
			{
				bool achieved = false;

				if (!m_achieved)
				{
					if (ISteamUserStats* pSteamUserStats = SteamUserStats())
					{
						achieved = pSteamUserStats->SetAchievement(m_name);
						if (achieved)
						{
							m_achieved = true;
							m_stats.Upload();
						}
					}
				}

				return achieved;
			}
		}
	}
}