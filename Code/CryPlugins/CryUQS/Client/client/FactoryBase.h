// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client
	{
		namespace internal
		{

			//===================================================================================
			//
			// CFactoryBase<>
			//
			//===================================================================================

			template <class TFactory>		// to be used as the curiously recurring template pattern
			class CFactoryBase
			{
			public:
				template <class TFactoryInterface>
				static void                                 RegisterAllInstancesInFactoryDatabase(core::IFactoryDatabase<TFactoryInterface>& databaseToRegisterInstancesIn);

			protected:
				explicit                                    CFactoryBase(const char* name);
				const char*                                 GetName() const;

			private:
				UQS_NON_COPYABLE(CFactoryBase);

			private:
				string                                      m_name;
				TFactory*                                   m_pNext;
				static TFactory*                            s_pList;
			};

			template <class TFactory>
			TFactory* CFactoryBase<TFactory>::s_pList;

			template <class TFactory>
			CFactoryBase<TFactory>::CFactoryBase(const char* name)
				: m_name(name)
			{
				// notice: we silently accept possible duplicates here (the factory-database will do such consistency checks)
				m_pNext = s_pList;
				s_pList = static_cast<TFactory*>(this);
			}

			template <class TFactory>
			const char* CFactoryBase<TFactory>::GetName() const
			{
				return m_name.c_str();
			}

			template <class TFactory>
			template <class TFactoryInterface>
			void CFactoryBase<TFactory>::RegisterAllInstancesInFactoryDatabase(core::IFactoryDatabase<TFactoryInterface>& databaseToRegisterInstancesIn)
			{
				for (TFactory* pCur = s_pList; pCur; pCur = pCur->m_pNext)
				{
					databaseToRegisterInstancesIn.RegisterFactory(pCur, pCur->m_name.c_str());
				}
			}

		}
	}
}
