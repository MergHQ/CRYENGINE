// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#ifndef FencedBuffer_H
	#define FencedBuffer_H

	#include "DevBuffer.h"

///////////////////////////////////////////////////////////////////////////////
// Video Buffer container optimized for direct VideoMemory access on Consoles
// No driver overhead, the lock function returns a direct pointer into Video Memory
// which is used by the GPU
// *NOTE: The programmer has to ensure that the Video Memory is not overwritten
//				while being used. For this the container provides additional fence and
//				wait for fence functions. Also double buffering of the container could be needed
// *NOTE: On non console platforms, this container is using the driver facilities to ensure
//				no memory is overwrite. This could mean additional memory allocated by the driver

class FencedBuffer
{
public:
	FencedBuffer(uint elemsCount, uint stride, DXGI_FORMAT format, uint32 flags);
	~FencedBuffer();

	byte*   Lock();
	void    Unlock();

	HRESULT BindVB(uint streamNumber = 0, int bytesOffset = 0, int stride = 0);
	HRESULT BindIB(uint offset);
	void    BindSRV(CDeviceManager::SHADER_TYPE shaderType, uint slot);

	uint    GetElemCount() const { return m_elemCount; }
	uint    GetStride() const    { return m_stride; }

	void    SetFence();
	void    WaitForFence();

private:
	CGpuBuffer        m_buffer;
	byte*             m_pLockedData;
	DeviceFenceHandle m_fence;
	uint              m_elemCount;
	uint              m_stride;
	uint              m_flags;
};

#endif  // FencedBuffer_H
