// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
*************************************************************************/

#include "StdAfx.h"
#include "XMLCPB_BufferReader.h"
#include "XMLCPB_Reader.h"

using namespace XMLCPB;

//////////////////////////////////////////////////////////////////////////

void SBufferReader::ReadFromFile(CReader& Reader, IPlatformOS::ISaveReaderPtr& pOSSaveReader, uint32 readSize)
{
	CRY_ASSERT(m_bufferSize == 0);
	CRY_ASSERT(!m_pBuffer);

	m_pBuffer = (uint8*)m_pHeap->Malloc(readSize, "");

	m_bufferSize = readSize;

	Reader.ReadDataFromFile(pOSSaveReader, const_cast<uint8*>(GetPointer(0)), readSize);
}

//////////////////////////////////////////////////////////////////////////

void SBufferReader::ReadFromMemory(CReader& Reader, const uint8* pData, uint32 dataSize, uint32 readSize, uint32& outReadLoc)
{
	CRY_ASSERT(m_bufferSize == 0);
	CRY_ASSERT(!m_pBuffer);

	m_pBuffer = (uint8*)m_pHeap->Malloc(readSize, "");

	m_bufferSize = readSize;

	Reader.ReadDataFromMemory(pData, dataSize, const_cast<uint8*>(GetPointer(0)), readSize, outReadLoc);
}
