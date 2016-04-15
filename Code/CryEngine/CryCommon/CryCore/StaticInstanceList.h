// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------

   Description: Static instance list. A utility class for automatic
             registration of types/data during static initialization.

   -------------------------------------------------------------------------
   History:
   - 08:06:2012: Created by Paul Slinger

*************************************************************************/

#ifndef __STATICINSTANCELIST_H__
#define __STATICINSTANCELIST_H__

template<typename TYPE> class CStaticInstanceList
{
public:

	inline CStaticInstanceList()
	{
		SInstanceList& instanceList = GetInstanceList();

		m_pNextInstance = instanceList.pFirstInstance;
		instanceList.pFirstInstance = this;
	}

	virtual ~CStaticInstanceList() {}

	static inline TYPE* GetFirstInstance()
	{
		return static_cast<TYPE*>(GetInstanceList().pFirstInstance);
	}

	inline TYPE* GetNextInstance() const
	{
		return static_cast<TYPE*>(m_pNextInstance);
	}

private:

	struct SInstanceList
	{
		inline SInstanceList()
			: pFirstInstance(NULL)
		{}

		CStaticInstanceList* pFirstInstance;
	};

	static inline SInstanceList& GetInstanceList()
	{
		static SInstanceList instanceList;
		return instanceList;
	}

	CStaticInstanceList* m_pNextInstance;
};

#endif //__STATICINSTANCELIST_H__
