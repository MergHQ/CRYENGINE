// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   SimpleStringPool.h
//  Created:     21/04/2006 by Timur.
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SimpleStringPool_h__
#define __SimpleStringPool_h__
#pragma once

#include <CrySystem/ISystem.h>

#include <CryCore/StlUtils.h>

//TODO: Pull most of this into a cpp file!

struct SStringData
{
	SStringData(const char* szString, int nStrLen)
		: m_szString(szString)
		, m_nStrLen(nStrLen)
	{
	}

	const char* m_szString;
	int         m_nStrLen;

private:
	bool operator==(const SStringData& other) const;
};

struct hash_stringdata
{
	size_t operator()(const SStringData& key) const
	{
		return stl::hash_strcmp<const char*>()(key.m_szString);
	}

	bool operator()(const SStringData& key1, const SStringData& key2) const
	{
		return
		  key1.m_nStrLen == key2.m_nStrLen &&
		  strcmp(key1.m_szString, key2.m_szString) == 0;
	}
};

/////////////////////////////////////////////////////////////////////
// String pool implementation.
// Inspired by expat implementation.
/////////////////////////////////////////////////////////////////////
class CSimpleStringPool
{
public:
	enum { STD_BLOCK_SIZE = 1u << 16 };
	struct BLOCK
	{
		BLOCK* next;
		int    size;
		char   s[1];
	};
	unsigned int m_blockSize;
	BLOCK*       m_blocks;
	BLOCK*       m_free_blocks;
	const char*  m_end;
	char*        m_ptr;
	char*        m_start;
	int          nUsedSpace;
	int          nUsedBlocks;
	bool         m_reuseStrings;

	typedef std::unordered_map<SStringData, char*, hash_stringdata, hash_stringdata> TStringToExistingStringMap;
	TStringToExistingStringMap m_stringToExistingStringMap;

	static size_t              g_nTotalAllocInXmlStringPools;

	CSimpleStringPool();

	explicit CSimpleStringPool(bool reuseStrings);

	~CSimpleStringPool();
	void SetBlockSize(unsigned int nBlockSize);
	void Clear();
	char* Append(const char* ptr, int nStrLen);
	char* ReplaceString(const char* str1, const char* str2);

	void GetMemoryUsage(ICrySizer* pSizer) const;

private:
	CSimpleStringPool(const CSimpleStringPool&);
	CSimpleStringPool& operator=(const CSimpleStringPool&);

private:
	void AllocBlock(int blockSize, int nMinBlockSize);
	void ReallocBlock(int blockSize);

	char* FindExistingString(const char* szString, int nStrLen) const;
};

#endif // __SimpleStringPool_h__
