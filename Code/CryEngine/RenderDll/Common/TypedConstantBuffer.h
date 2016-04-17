// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "DevBuffer.h"
#include "DriverD3D.h"

template<typename T>
class CTypedConstantBuffer
{
protected:
	T                  m_hostBuffer;
	CConstantBufferPtr m_constantBuffer;

public:
	CTypedConstantBuffer() { ZeroStruct(m_hostBuffer); }
	CTypedConstantBuffer(const CTypedConstantBuffer<T>& cb) : m_constantBuffer(nullptr) { m_hostBuffer = cb.m_hostBuffer; }
	CTypedConstantBuffer(CConstantBufferPtr incb) : m_constantBuffer(incb) {}

	bool               IsDeviceBufferAllocated() { return m_constantBuffer != nullptr; }
	CConstantBufferPtr GetDeviceConstantBuffer()
	{
		if (!m_constantBuffer)
		{
			CreateDeviceBuffer();
		}
		return m_constantBuffer;
	}

	void CreateDeviceBuffer()
	{
		int size = sizeof(T);
		m_constantBuffer.Assign_NoAddRef(gcpRendD3D->m_DevBufMan.CreateConstantBuffer(size)); // TODO: this could be a good candidate for dynamic=false
		CopyToDevice();
	}
	void CopyToDevice()
	{
		m_constantBuffer->UpdateBuffer(&m_hostBuffer, sizeof(m_hostBuffer));
	}

	T*       operator->()       { return &m_hostBuffer; }
	const T* operator->() const { return &m_hostBuffer; }

	T&       operator=(const T& hostData)
	{
		return m_hostBuffer = hostData;
	}
};
