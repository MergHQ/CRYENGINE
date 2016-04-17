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
#ifndef __CRYDX12__
	#define __CRYDX12__

	#include "CryDX12Legacy.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern int g_nPrintDX12;

	#ifdef _DEBUG
		#define DX12_LOG(cond, ...) \
		  do { if (cond || g_nPrintDX12) { CryLog("DX12 Log: " __VA_ARGS__); } } while (0)
		#define DX12_ERROR(...) \
		  do { CryLog("DX12 Error: " __VA_ARGS__); } while (0)
		#define DX12_ASSERT(cond, ...) \
		  do { if (!(cond)) { DX12_ERROR(__VA_ARGS__); CRY_ASSERT(0); __debugbreak(); } } while (0)
		#define DX12_WARNING(cond, ...) \
		  do { if (!(cond)) { DX12_LOG(__VA_ARGS__); } } while (0)
	#else
		#define DX12_LOG(cond, ...) do {} while (0)
		#define DX12_ERROR(...)     do {} while (0)
		#define DX12_ASSERT(cond, ...)
		#define DX12_WARNING(cond, ...)
	#endif

	#define DX12_NOT_IMPLEMENTED DX12_ASSERT(0, "Not implemented!");

	#include "API/DX12Base.hpp"
	#include "API/DX12PSO.hpp"
	#include "Misc/SCryDX11PipelineState.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI DX12CreateDXGIFactory1(REFIID riid, void** ppFactory);

HRESULT WINAPI DX12CreateDevice(
  IDXGIAdapter* pAdapter,
  D3D_DRIVER_TYPE DriverType,
  HMODULE Software,
  UINT Flags,
  CONST D3D_FEATURE_LEVEL* pFeatureLevels,
  UINT FeatureLevels,
  UINT SDKVersion,
  ID3D11Device** ppDevice,
  D3D_FEATURE_LEVEL* pFeatureLevel,
  ID3D11DeviceContext** ppImmediateContext);

struct SSamplerHash
{
	uint32_t                    m_nHash;
	int32_t                     m_nOffset;
	D3D12_CPU_DESCRIPTOR_HANDLE m_srcHandle;
};

struct SSamplerGroup
{
	std::vector<SSamplerHash> m_Samplers;
	int                       m_nSmpHeapOffset;
};

struct SDescriptorBlock;

struct SHeapCB
{
	int32 m_nOffs;
	int32 m_nSlot;
};
struct SInstancingHeap
{
	CCryDX12Buffer* m_pBuffer;
	TRange<UINT>    m_BindRange;
	uint32          m_nOffs;
	uint32          m_nInstances;
};
struct SStateHeap
{
	//int m_nResHeapID[2];
	//int m_nResHeapOffset;
	int                            m_nResHeapSize;

	int                            m_nSamplerGroup;

	std::vector<SInstancingHeap>   m_InstancingHeap;
	std::vector<SHeapCB>           m_HeapCBs;
	std::vector<SDescriptorBlock*> m_pHeapBlocks;

	SStateHeap()
	{
		m_nSamplerGroup = 0;
		m_nResHeapSize = 0;
	}
};

#endif // __CRYDX12__
