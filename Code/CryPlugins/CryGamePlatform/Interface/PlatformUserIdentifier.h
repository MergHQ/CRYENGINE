// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "PlatformTypes.h"

namespace Cry
{
	namespace GamePlatform
	{
		//! Identifies a game platform user. Essentially, it groups
		//! all of the user's accounts across different services.
		class UserIdentifier
		{
		public:
			explicit UserIdentifier(const AccountIdentifier& mainAccountId)
				: m_mainAccountId(mainAccountId)
			{
			}

			UserIdentifier() = default;

			UserIdentifier(const UserIdentifier&) = default;
			UserIdentifier(UserIdentifier&&) = default;

			UserIdentifier& operator=(const UserIdentifier&) = default;
			UserIdentifier& operator=(UserIdentifier&&) = default;

			const char* ToDebugString() const
			{
				return m_mainAccountId.ToDebugString();
			}

			bool operator==(const UserIdentifier& other) const { return m_mainAccountId == other.m_mainAccountId; }
			bool operator!=(const UserIdentifier& other) const { return m_mainAccountId != other.m_mainAccountId; }

		private:
			AccountIdentifier m_mainAccountId;
		};
	}
}
