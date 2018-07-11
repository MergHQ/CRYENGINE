// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace Cry
{
	namespace GamePlatform
	{
		// Forward declarations
		template <typename Traits>
		class Identifier;

		template <typename Traits>
		bool Serialize(Serialization::IArchive& archive, Identifier<Traits>& value, const char* szName, const char* szLabel);


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

			friend bool Serialize<Traits>(Serialization::IArchive& archive, Identifier<Traits>& value, const char* szName, const char* szLabel);

		private:
			ServiceType m_svcId;
			ValueType m_value;
		};

		template <typename Traits>
		bool Serialize(Serialization::IArchive& archive, Identifier<Traits>& value, const char* szName, const char* szLabel)
		{
			return archive(value.m_value, szName, szLabel);
		}
	}
}
