// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:
//  Version:     v1.00
//  Created:     03/02/2015 by Jan Pinter
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef __DX12BASE__
	#define __DX12BASE__

#if defined(CRY_USE_DX12) && CRY_PLATFORM_DURANGO

	#include <d3d11_x.h>
	#include <d3d12_x.h>
	#include "..\Includes\d3dx12.h"

	#include "DX12\Includes\d3d11_empty.h"

#elif defined(DURANGO_MONOD3D_DRIVER)
	//#include <d3d11_x.h>

	#include <d3d11_x.h>
	#include <d3d12_x.h>
	#include "..\Includes\d3dx12.h"
	//	#include <d3dx12_x.h>

#else
	#include <d3d11.h>
	#include <d3d11_1.h>
	#include <dxgi1_2.h>

	#include <d3d12.h>
	#include <d3dx12.h>
	#include <dxgi1_4.h>

	#include "DX12\Includes\d3d11_empty.h"
#endif


	#include <CryMath/Range.h>

	#include "xxhash.h"
	#include <fasthash/fasthash.inl>
	#include <concqueue/concqueue.hpp>

	#define threadsafe
	#define threadsafe_const const

extern int g_nPrintDX12;

	#define DX12_PTR(T)        _smart_ptr < T >
	#define DX12_NEW_RAW(ctor) NCryDX12::PassAddRef(new ctor)

	#ifdef _DEBUG

		#define DX12_FUNC_LOG do {                        \
		  if (g_nPrintDX12)                               \
		  { CryLog("DX12 function call: %s", __FUNC__); } \
	} while (0);

	#else
		#define DX12_FUNC_LOG do {} while (0);
	#endif

	#if     CRY_PLATFORM_64BIT
		#define CRY_USE_DX12_MULTIADAPTER            true
		#define CRY_USE_DX12_MULTIADAPTER_SIMULATION 0
	#endif

	#define DX12_PRECISE_DEDUPLICATION
	#define	DX12_MAP_STAGE_MULTIGPU       1
	#define DX12_MAP_DISCARD_MARKER       BIT(3)
	#define DX12_COPY_REVERTSTATE_MARKER  BIT(2)
	#define DX12_COPY_PIXELSTATE_MARKER   BIT(3)
	#define DX12_RESOURCE_FLAG_OVERLAP    BIT(1)
	#define DX12_GPU_VIRTUAL_ADDRESS_NULL 0ULL
	#define INVALID_CPU_DESCRIPTOR_HANDLE CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT)
	#define INVALID_GPU_DESCRIPTOR_HANDLE CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT)
	#define DX12_CONCURRENCY_ANALYZER     false
	#define DX12_FENCE_ANALYZER           false
	#define DX12_BARRIER_ANALYZER         false

// Extract lowest set isolated bit "intrinsic" -> _blsi_u32 (Jaguar == PS4|XO, PileDriver+, Haswell+)
	#define blsi(field) (field & (-static_cast<INT>(field)))

namespace NCryDX12
{

// Forward declarations
class CRefCounted;
class CCommandList;
class CCommandListPool;
class CDescriptorHeap;
class CDevice;
class CGraphicsPSO;
class CPSOCache;
class CResource;
class CView;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
template<typename T>
static T* PassAddRef(T* ptr)
{
	if (ptr)
	{
		ptr->AddRef();
	}

	return ptr;
}

//---------------------------------------------------------------------------------------------------------------------
template<typename T>
static T* PassAddRef(const _smart_ptr<T>& ptr)
{
	if (ptr)
	{
		ptr.get()->AddRef();
	}

	return ptr.get();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef uint32_t THash;
template<size_t length>
ILINE THash ComputeSmallHash(const void* data, UINT seed = 666)
{
	return fasthash::fasthash32<length>(data, seed);
}

//---------------------------------------------------------------------------------------------------------------------

struct NodeMaskCV
{
	UINT creationMask;
	UINT visibilityMask;
};

typedef struct NodeMaskCV NODE64;

//---------------------------------------------------------------------------------------------------------------------
UINT              GetDXGIFormatSize(DXGI_FORMAT format);
D3D12_CLEAR_VALUE GetDXGIFormatClearValue(DXGI_FORMAT format, bool depth);

ILINE bool        IsDXGIFormatCompressed(DXGI_FORMAT format)
{
	return
	  (format >= DXGI_FORMAT_BC1_TYPELESS && format <= DXGI_FORMAT_BC5_SNORM) ||
	  (format >= DXGI_FORMAT_BC6H_TYPELESS && format <= DXGI_FORMAT_BC7_UNORM_SRGB);
}

ILINE bool IsDXGIFormatCompressed4bpp(DXGI_FORMAT format)
{
	return
	  (format >= DXGI_FORMAT_BC1_TYPELESS && format <= DXGI_FORMAT_BC1_UNORM_SRGB) ||
	  (format >= DXGI_FORMAT_BC4_TYPELESS && format <= DXGI_FORMAT_BC4_SNORM);
}

ILINE bool IsDXGIFormatCompressed8bpp(DXGI_FORMAT format)
{
	return
	  (format >= DXGI_FORMAT_BC2_TYPELESS && format <= DXGI_FORMAT_BC3_UNORM_SRGB) ||
	  (format >= DXGI_FORMAT_BC5_TYPELESS && format <= DXGI_FORMAT_BC5_SNORM) ||
	  (format >= DXGI_FORMAT_BC6H_TYPELESS && format <= DXGI_FORMAT_BC7_UNORM_SRGB);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CRefCounted
{
	int m_RefCount;

public:
	CRefCounted() : m_RefCount(0)
	{}

	CRefCounted(CRefCounted&& r) : m_RefCount(std::move(r.m_RefCount))
	{}

	CRefCounted& operator=(CRefCounted&& r)
	{ m_RefCount = std::move(r.m_RefCount); return *this; }

	void AddRef() threadsafe
	{
		CryInterlockedIncrement(&m_RefCount);
	}
	void Release() threadsafe
	{
		if (!CryInterlockedDecrement(&m_RefCount))
		{
			delete this;
		}
	}
	void ReleaseNoDelete() threadsafe
	{
		CryInterlockedDecrement(&m_RefCount);
	}

protected:
	virtual ~CRefCounted() {}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceObject : public CRefCounted
{
	CDevice* m_pDevice;

public:
	ILINE CDevice* GetDevice() const
	{
		return m_pDevice;
	}

	ILINE bool IsInitialized() const
	{
		return m_IsInitialized;
	}

protected:
	CDeviceObject(CDevice* device);
	virtual ~CDeviceObject();

	CDeviceObject(CDeviceObject&& r)
		: CRefCounted(std::move(r))
		, m_IsInitialized(std::move(r.m_IsInitialized))
		, m_pDevice(std::move(r.m_pDevice))
	{}

	CDeviceObject& operator=(CDeviceObject&& r)
	{
		CRefCounted::operator=(std::move(r));

		m_IsInitialized = std::move(r.m_IsInitialized);
		m_pDevice = std::move(r.m_pDevice);

		return *this;
	}

	ILINE void SetDevice(CDevice* device)
	{
		m_pDevice = device;
	}

	ILINE void IsInitialized(bool is)
	{
		m_IsInitialized = is;
	}

private:
	bool m_IsInitialized;
};

static const UINT CONSTANT_BUFFER_ELEMENT_SIZE = 16U;
}

	#include "DX12Device.hpp"

#endif // __DX12BASE__
