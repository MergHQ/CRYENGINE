// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace Cry
{
	namespace GamePlatform
	{
		//! Simple opaque class used to store a service-specific identifier.
		template <typename Traits>
		class Identifier
		{
			using ValueType = typename Traits::ValueType;
			using ServiceType = typename Traits::ServiceType;
		public:
			Identifier(const ServiceType& svcId, ValueType accountId)
				: m_svcId(svcId)
				, m_value(accountId)
			{
			}

			Identifier()
				: m_svcId(CryGUID::Null())
				, m_value(0)
			{
			}

			Identifier(const Identifier&) = default;
			Identifier(Identifier&&) = default;

			Identifier& operator=(const Identifier&) = default;
			Identifier& operator=(Identifier&&) = default;

			const ServiceType& Service() const { return m_svcId; }

			void Serialize(Serialization::IArchive& archive)
			{
				archive(m_value);
			}

			bool operator==(const Identifier& other) const { return m_svcId == other.m_svcId && m_value == other.m_value; }
			bool operator!=(const Identifier& other) const { return m_svcId != other.m_svcId || m_value != other.m_value; }
			bool operator<(const Identifier& other) const
			{
				if (m_svcId < other.m_svcId)
				{
					return true;
				}
				else if (m_svcId == other.m_svcId)
				{
					return m_value < other.m_value;
				}

				return false;
			}

			const char* ToDebugString() const
			{
				return Traits::ToDebugString(m_svcId, m_value);
			}

		private:
			ServiceType m_svcId;
			ValueType m_value;
		};
	}
}
