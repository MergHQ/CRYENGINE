// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GpuBitonicSort.h"

namespace gpu
{

CBitonicSort::CBitonicSort() : m_data(NUM_ELEMENTS), m_transposeData(NUM_ELEMENTS), m_backend("BitonicSort")
{
	m_params.CreateDeviceBuffer();
}

void CBitonicSort::Sort(size_t numElements)
{
	PROFILE_LABEL_SCOPE("Bitonic Sort");

	// Note: Minimum elements must be fixed !!!!!!!!!!!
	numElements = min(numElements, (size_t)NUM_ELEMENTS);
	const size_t MATRIX_WIDTH = BITONIC_BLOCK_SIZE;
	const size_t MATRIX_HEIGHT = numElements / BITONIC_BLOCK_SIZE;

	// First sort the rows for the levels <= to the block size
	m_params.Bind();
	{
		ID3D11UnorderedAccessView* uavs[] = { m_data.GetUAV(), m_transposeData.GetUAV() };
		m_backend.SetUAVs(0, 2, uavs);
	}

	for (size_t level = 2; level <= BITONIC_BLOCK_SIZE; level <<= 1)
	{
		SyncParams(level, level, MATRIX_HEIGHT, MATRIX_WIDTH);
		m_params.Bind();
		m_backend.RunTechnique("BitonicSort", numElements / BITONIC_BLOCK_SIZE, 1, 1);
	}

	// Then sort the rows and columns for the levels > than the block size
	// Transpose. Sort the Columns. Transpose. Sort the Rows.
	for (size_t level = (BITONIC_BLOCK_SIZE << 1); level <= numElements; level <<= 1)
	{
		// Transpose the data from buffer 1 into buffer 2
		SyncParams((level / BITONIC_BLOCK_SIZE), (level & ~numElements) / BITONIC_BLOCK_SIZE,
		           MATRIX_WIDTH, MATRIX_HEIGHT);
		{
			ID3D11UnorderedAccessView* uavs[] = { m_transposeData.GetUAV(), m_data.GetUAV() };
			m_backend.SetUAVs(0, 2, uavs);
		}

		m_params.Bind();

		m_backend.RunTechnique("BitonicTranspose", MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, 1);

		// Sort the transposed column data
		m_backend.RunTechnique("BitonicSort", numElements / BITONIC_BLOCK_SIZE, 1, 1);

		// Transpose the data from buffer 2 back into buffer 1
		SyncParams(BITONIC_BLOCK_SIZE, level, MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE);
		{
			ID3D11UnorderedAccessView* uavs[] = { m_data.GetUAV(), m_transposeData.GetUAV() };
			m_backend.SetUAVs(0, 2, uavs);
		}

		m_params.Bind();
		m_backend.RunTechnique("BitonicTranspose", MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, 1);

		// Sort the row data
		m_backend.RunTechnique("BitonicSort", numElements / BITONIC_BLOCK_SIZE, 1, 1);
	}
}

void CBitonicSort::SyncParams(uint32 iLevel, uint32 iLevelMask, uint32 iWidth, uint32 iHeight)
{
	m_params->iLevel = iLevel;
	m_params->iLevelMask = iLevelMask;
	m_params->iWidth = iWidth;
	m_params->iHeight = iHeight;
	m_params.CopyToDevice();
}

}
