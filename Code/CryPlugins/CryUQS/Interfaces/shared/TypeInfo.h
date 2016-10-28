// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace shared
	{

		//===================================================================================
		//
		// CTypeInfo
		//
		//===================================================================================

		class CTypeInfo
		{
			template <class T>
			friend struct SDataTypeHelper;	// calls CTypeInfo::Get<T>()

		public:
			const char*             name() const;
			bool                    operator==(const CTypeInfo& rhs) const;
			bool                    operator!=(const CTypeInfo& rhs) const;
			bool                    operator<(const CTypeInfo& rhs) const;

		private:

			explicit                CTypeInfo(const yasli::TypeID& yasliTypeID);

			template <class T>
			static const CTypeInfo& Get();

		private:
			const yasli::TypeID     m_yasliTypeID;
		};

		inline CTypeInfo::CTypeInfo(const yasli::TypeID& yasliTypeID)
			: m_yasliTypeID(yasliTypeID)
		{}

		inline const char* CTypeInfo::name() const
		{
			return m_yasliTypeID.name();
		}

		inline bool CTypeInfo::operator==(const CTypeInfo& rhs) const
		{
			return m_yasliTypeID == rhs.m_yasliTypeID;
		}

		inline bool CTypeInfo::operator!=(const CTypeInfo& rhs) const
		{
			return !operator==(rhs);
		}

		inline bool CTypeInfo::operator<(const CTypeInfo& rhs) const
		{
			return m_yasliTypeID < rhs.m_yasliTypeID;
		}

		template <class T>
		const CTypeInfo& CTypeInfo::Get()
		{
			static const CTypeInfo type(yasli::TypeID::get<T>());
			return type;
		}

	}
}
