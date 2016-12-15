// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------

   Description: Static instance list. A utility class for automatic
             registration of types/data during static initialization.

   -------------------------------------------------------------------------
   History:
   - 08:06:2012: Created by Paul Slinger

*************************************************************************/

#pragma once

template<typename TYPE> class CStaticInstanceList
{
private:

	struct SInstanceList
	{
		CStaticInstanceList* pFirstInstance = nullptr;
	};

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

	static inline SInstanceList& GetInstanceList()
	{
		static SInstanceList instanceList;
		return instanceList;
	}

private:

	CStaticInstanceList* m_pNextInstance;
};