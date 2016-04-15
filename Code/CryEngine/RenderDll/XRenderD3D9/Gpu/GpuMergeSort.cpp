// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     05/12/2015 by Benjamin Block
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GpuMergeSort.h"

namespace gpu
{

CMergeSort::CMergeSort(int maxElements) : m_data(maxElements), m_pResultSRV(nullptr), m_maxElements(maxElements), m_backend("GpuMergeSort")
{
	m_params.CreateDeviceBuffer();
	m_data.Initialize(true);
}

void CMergeSort::Sort(size_t numElements)
{
	PROFILE_LABEL_SCOPE("GPU MERGE SORT");

	CRY_ASSERT(numElements <= m_maxElements);

	int block = 1;
	const int blocks = gpu::GetNumberOfBlocksForArbitaryNumberOfThreads(numElements, 1024);

	while (block < numElements)
	{
		ID3D11UnorderedAccessView* pUAVs[] = { m_data.Get().GetUAV(), m_data.GetBackBuffer().GetUAV() };
		SyncParams(block);
		m_backend.SetUAVs(0, 2, pUAVs);
		m_params.Bind();
		m_backend.RunTechnique("MergeSort", blocks, 1, 1);
		m_data.Swap();
		block *= 2;
	}
	m_pResultSRV = m_data.Get().GetSRV();
}

void CMergeSort::SyncParams(uint inputBlockSize)
{
	m_params->inputBlockSize = inputBlockSize;
	m_params.CopyToDevice();
}

}
