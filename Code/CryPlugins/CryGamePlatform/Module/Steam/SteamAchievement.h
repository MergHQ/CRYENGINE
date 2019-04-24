// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IPlatformAchievement.h"
#include "SteamTypes.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Steam
		{
			class CAchievement
				: public IAchievement
			{
			public:
				CAchievement(CStatistics& steamStats, const char* name, bool achieved);
				virtual ~CAchievement() = default;

				// IAchievement
				virtual const char* GetName() const override { return m_name.c_str(); }
				virtual bool IsCompleted() const override { return m_achieved; }

				virtual bool Reset() override;
				virtual bool Achieve() override;
				// ~IAchievement

			protected:
				CStatistics& m_stats;
				string m_name;
				bool m_achieved;
			};
		}
	}
}