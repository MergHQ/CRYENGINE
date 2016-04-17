// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     05/12/2015 by Benjamin Block
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "GpuComputeBackend.h"

namespace gpu
{
class CMergeSort
{
public:
	struct SMergeSortItem
	{
		uint32 key;
		uint32 payload;
	};

	struct CParams
	{
		int32 inputBlockSize;
	};

	CMergeSort(int maxElements);
	// numElements needs to be power-of-two and <= maxElements
	void                       Sort(size_t numElements);
	ID3D11UnorderedAccessView* GetUAV()       { return m_data.Get().GetUAV(); }

	ID3D11ShaderResourceView*  GetResultSRV() { return m_pResultSRV; }

private:
	void SyncParams(uint inputBlockSize);
	int m_maxElements;
	gpu::CTypedConstantBuffer<CParams, 8> m_params;
	gpu::CDoubleBuffered<gpu::CTypedResource<SMergeSortItem, gpu::BufferFlagsReadWriteReadback>> m_data;
	gpu::CComputeBackend                  m_backend;
	ID3D11ShaderResourceView*             m_pResultSRV;
};
}
