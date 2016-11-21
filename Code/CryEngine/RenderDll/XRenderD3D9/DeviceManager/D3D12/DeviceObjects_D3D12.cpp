// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryCore/CryCustomTypes.h>
#include "../../DeviceManager/DeviceObjects.h"
#include "DriverD3D.h"

#if defined(CRY_USE_DX12)
	#include <pix_win.h>

	#define CRY_USE_DX12_NATIVE
#endif

#if defined(CRY_USE_DX12_NATIVE)
#include "DX12/API/Redirections/D3D12Device.inl"

using namespace NCryDX12;
D3D12_SHADER_BYTECODE g_EmptyShader;

	#define GET_DX12_TEXTURE_VIEW(uniformTexture, uniformView) reinterpret_cast<CCryDX12ShaderResourceView*>((uniformTexture)->GetShaderResourceView(uniformView))
	#define GET_DX12_BUFFER_VIEW(uniformBuffer)                reinterpret_cast<CCryDX12ShaderResourceView*>((uniformBuffer).GetSRV())
	#define GET_DX12_UNORDERED_VIEW(uniformBufferOrTexture)    reinterpret_cast<CCryDX12UnorderedAccessView*>((uniformBufferOrTexture)->GetDeviceUAV())
	#define GET_DX12_DEPTHSTENCIL_VIEW(dsTarget)               reinterpret_cast<CCryDX12DepthStencilView*>((dsTarget)->pSurface)
	#define GET_DX12_RENDERTARGET_VIEW(rTarget, rView)         reinterpret_cast<CCryDX12RenderTargetView*>((rTarget)->GetResourceView(rView))

	#define GET_DX12_TEXTURE_RESOURCE(uniformTexture)          reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>((uniformTexture)->GetDevTexture()->GetBaseTexture())
	#define GET_DX12_BUFFER_RESOURCE(uniformBuffer)            reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>((uniformBuffer).GetBuffer())
	#define GET_DX12_CONSTANTBUFFER_RESOURCE(constantBuffer)   reinterpret_cast<CCryDX12Buffer*>((constantBuffer)->m_buffer)
	#define GET_DX12_SAMPLER(uniformSampler)                   reinterpret_cast<CCryDX12SamplerState*>((uniformSampler).m_pDeviceState)

static NCryDX12::CDevice* GetDevice()
{
	return reinterpret_cast<CCryDX12Device*>(gcpRendD3D->GetDevice().GetRealDevice())->GetDX12Device();
}

D3D12_SHADER_RESOURCE_VIEW_DESC GetNullSrvDesc(const CTexture* pTexture)
{
	D3D12_SRV_DIMENSION viewDimension[eTT_MaxTexType] =
	{
		D3D12_SRV_DIMENSION_TEXTURE1D,                      // eTT_1D
		D3D12_SRV_DIMENSION_TEXTURE2D,                      // eTT_2D
		D3D12_SRV_DIMENSION_TEXTURE3D,                      // eTT_3D
		D3D12_SRV_DIMENSION_TEXTURECUBE,                    // eTT_Cube
		D3D12_SRV_DIMENSION_TEXTURECUBEARRAY,               // eTT_CubeArray
		D3D12_SRV_DIMENSION_TEXTURE2D,                      // eTT_Dyn2D
		D3D12_SRV_DIMENSION_TEXTURE2D,                      // eTT_User
		D3D12_SRV_DIMENSION_TEXTURECUBE,                    // eTT_NearestCube
		D3D12_SRV_DIMENSION_TEXTURE2DARRAY,                 // eTT_2DArray
		D3D12_SRV_DIMENSION_TEXTURE2DMS,                    // eTT_2DMS
		D3D12_SRV_DIMENSION_TEXTURE2D,                      // eTT_Auto2D
	};

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroStruct(srvDesc);

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = pTexture ? viewDimension[pTexture->GetTexType()] : D3D12_SRV_DIMENSION_TEXTURE2D;

	return srvDesc;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC GetNullUavDesc(const CTexture* pTexture)
{
	D3D12_UAV_DIMENSION viewDimension[eTT_MaxTexType] =
	{
		D3D12_UAV_DIMENSION_TEXTURE1D,                      // eTT_1D
		D3D12_UAV_DIMENSION_TEXTURE2D,                      // eTT_2D
		D3D12_UAV_DIMENSION_TEXTURE3D,                      // eTT_3D
		D3D12_UAV_DIMENSION_UNKNOWN,                        // eTT_Cube
		D3D12_UAV_DIMENSION_UNKNOWN,                        // eTT_CubeArray
		D3D12_UAV_DIMENSION_TEXTURE2D,                      // eTT_Dyn2D
		D3D12_UAV_DIMENSION_TEXTURE2D,                      // eTT_User
		D3D12_UAV_DIMENSION_UNKNOWN,                        // eTT_NearestCube
		D3D12_UAV_DIMENSION_TEXTURE2DARRAY,                 // eTT_2DArray
		D3D12_UAV_DIMENSION_UNKNOWN,                        // eTT_2DMS
		D3D12_UAV_DIMENSION_TEXTURE2D,                      // eTT_Auto2D
	};

	D3D12_UNORDERED_ACCESS_VIEW_DESC result;
	ZeroStruct(result);
	result.ViewDimension = pTexture ? viewDimension[pTexture->GetTexType()] : D3D12_UAV_DIMENSION_TEXTURE2D;
	result.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	return result;
}

D3D12_SHADER_RESOURCE_VIEW_DESC GetNullSrvDesc(const CGpuBuffer& buffer)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroStruct(srvDesc);
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32_UINT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

	return srvDesc;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC GetNullUavDesc(const CGpuBuffer& buffer)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	ZeroStruct(uavDesc);
	uavDesc.Format = DXGI_FORMAT_R32_UINT;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

	return uavDesc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceResourceSet_DX12 : public CDeviceResourceSet
{
public:
	friend CDeviceGraphicsCommandInterfaceImpl;
	friend CDeviceComputeCommandInterfaceImpl;

	CDeviceResourceSet_DX12(CDeviceResourceSet::EFlags flags)
		: CDeviceResourceSet(flags)
	{
		ZeroStruct(m_DescriptorInfo);
	}

	CDeviceResourceSet_DX12(const CDeviceResourceSet& other)
		: CDeviceResourceSet(other)
	{
		ZeroStruct(m_DescriptorInfo);
	}

	~CDeviceResourceSet_DX12()
	{
		ReleaseDescriptors();
	}

	virtual bool            BuildImpl(EFlags updatedFlags) final;

	const CDescriptorBlock& GetDescriptorBlock() const { return m_DescriptorBlock; }
	void                    MarkInUse();

protected:

	struct SResourcePatchingInfo
	{
		DeviceResourceBinding::SShaderSlot shaderSlot;
		int                                descriptorTableOffset;
		int                                inUseArrayOffset;
		bool                               bUnorderedAccess;

		union { CCryDX12ShaderResourceView* pSrv; CCryDX12UnorderedAccessView* pUav; };
	};

	struct SConstantBufferPatchingInfo
	{
		DeviceResourceBinding::SShaderSlot shaderSlot;
		int                                descriptorTableOffset;
		D3D12_CONSTANT_BUFFER_VIEW_DESC    desc;
	};

	struct SDescriptorSet
	{
		std::vector<SResourcePatchingInfo>            texturePatchingInfo;
		std::vector<SResourcePatchingInfo>            bufferPatchingInfo;
		std::vector<SConstantBufferPatchingInfo>      constantBufferPatchingInfo;

		CTrackedItemCustomAllocator<SDescriptorBlock> descriptorBlockAllocator;
		SDescriptorBlock*                             pCurrentDescriptorBlock;

		SDescriptorSet()
			: pCurrentDescriptorBlock(nullptr)
			, descriptorBlockAllocator(nullptr, nullptr)
		{
		}

		~SDescriptorSet()
		{
			Reset();
		}

		void Reset()
		{
			descriptorBlockAllocator.Reset();
			pCurrentDescriptorBlock = nullptr;

			texturePatchingInfo.clear();
			bufferPatchingInfo.clear();
			constantBufferPatchingInfo.clear();
		}
	};

protected:
	// Workaround for: "template<template<typename, class> class C, typename T> using CContainer = C<T, CSingleBlockAllocator<T>>;"
	typedef std::vector<D3D12_CPU_DESCRIPTOR_HANDLE, CryStack::CSingleBlockAllocator<D3D12_CPU_DESCRIPTOR_HANDLE>> StackDescriptorVector;
	const inline size_t NoAlign(size_t nSize) { return nSize; }

	void ReleaseDescriptors();

	template<bool bGatherResourcesInUse, bool bGatherPatchingInfo>
	bool GatherDescriptors(
	  StackDescriptorVector& descriptors,
	  CDescriptorBlock& descriptorScratchSpace,
	  std::vector<_smart_ptr<CCryDX12Buffer>>* pConstantBuffersInUse = nullptr,
	  std::vector<_smart_ptr<CCryDX12ShaderResourceView>>* pShaderResourceViewsInUse = nullptr,
	  std::vector<_smart_ptr<CCryDX12UnorderedAccessView>>* pUnorderedAccessViewsInUse = nullptr,
	  std::vector<SResourcePatchingInfo>* pTexturePatchingInfo = nullptr,
	  std::vector<SResourcePatchingInfo>* pBufferPatchingInfo = nullptr,
	  std::vector<SConstantBufferPatchingInfo>* pConstantBufferPatchingInfo = nullptr) const;

	void FillDescriptorBlock(
		StackDescriptorVector& descriptors,
		CDescriptorBlock& descriptorBlock) const;

	void PatchDescriptors(CDescriptorBlock& descriptorScratchSpace);

	CDescriptorBlock m_DescriptorBlock;

	union
	{
		SDescriptorSet*   pDescriptorSet;
		SDescriptorBlock* pDescriptors;
	} m_DescriptorInfo;

	std::vector<_smart_ptr<CCryDX12Buffer>>              m_ConstantBuffersInUse;
	std::vector<_smart_ptr<CCryDX12ShaderResourceView>>  m_ShaderResourceViewsInUse;
	std::vector<_smart_ptr<CCryDX12UnorderedAccessView>> m_UnorderedAccessViewsInUse;
};

void CDeviceResourceSet_DX12::MarkInUse()
{
	if (m_DescriptorInfo.pDescriptors)
	{
		auto pDescriptorBlock =
		  (m_Flags & EFlags_Multibuffered)
		  ? m_DescriptorInfo.pDescriptorSet->pCurrentDescriptorBlock
		  : m_DescriptorInfo.pDescriptors;

		if (pDescriptorBlock)
			pDescriptorBlock->MarkUsed();
	}
}

void CDeviceResourceSet_DX12::ReleaseDescriptors()
{
	if (m_Flags & EFlags_Multibuffered)
	{
		SAFE_DELETE(m_DescriptorInfo.pDescriptorSet);
	}
	else
	{
		if (m_DescriptorInfo.pDescriptors != nullptr)
		{
			gcpRendD3D->m_DevBufMan.ReleaseDescriptorBlock(m_DescriptorInfo.pDescriptors);
			m_DescriptorInfo.pDescriptors = nullptr;
		}
	}

	m_DescriptorBlock = CDescriptorBlock();
}

// requires no command-list, just a device
bool CDeviceResourceSet_DX12::BuildImpl(EFlags updatedFlags)
{
	CRY_ASSERT(gRenDev->m_pRT->IsRenderThread());
	auto pDevice = GetDevice();

	// allocate new descriptor block. NOTE: deletion of previous block is deferred until gpu has finished using it
	const bool bRequiresMultiBuffering = !!(updatedFlags & EFlags_Multibuffered);
	const bool bHasMultiBuffering      = !!(m_Flags      & EFlags_Multibuffered);
	const int descriptorCount = m_ConstantBuffers.size() + m_Textures.size() + m_Buffers.size();
	bool bBuildSuccess;

	CryStackAllocatorWithSizeVector(D3D12_CPU_DESCRIPTOR_HANDLE, descriptorCount, descriptorMem, NoAlign);
	StackDescriptorVector descriptors(descriptorMem);
	descriptors.reserve(descriptorCount);

	if (bRequiresMultiBuffering ^ bHasMultiBuffering)
	{
		ReleaseDescriptors();
	}

	if (bRequiresMultiBuffering)
	{
		if (!m_DescriptorInfo.pDescriptorSet)
			m_DescriptorInfo.pDescriptorSet = new SDescriptorSet();

		auto pDescriptors = m_DescriptorInfo.pDescriptorSet;
		pDescriptors->Reset();

		auto& allocator = pDescriptors->descriptorBlockAllocator;
		auto allocateBlock = [=]() { return gcpRendD3D->m_DevBufMan.CreateDescriptorBlock(descriptorCount); };
		auto releaseBlock = [](SDescriptorBlock*& pBlock) { gcpRendD3D->m_DevBufMan.ReleaseDescriptorBlock(pBlock); };

		allocator.SetAllocFunctions(allocateBlock, releaseBlock);
		pDescriptors->pCurrentDescriptorBlock = allocator.Allocate();

		m_DescriptorBlock = *pDescriptors->pCurrentDescriptorBlock;

		// gather descriptors for all bound resources and copy to new block. NOTE: CBV_SRV_UAV heap, SMP heap not yet supported
		bBuildSuccess = GatherDescriptors<true, true>(descriptors,
			pDevice->GetResourceDescriptorScratchSpace(),
			&m_ConstantBuffersInUse,
			&m_ShaderResourceViewsInUse,
			&m_UnorderedAccessViewsInUse,
			&m_DescriptorInfo.pDescriptorSet->texturePatchingInfo,
			&m_DescriptorInfo.pDescriptorSet->bufferPatchingInfo,
			&m_DescriptorInfo.pDescriptorSet->constantBufferPatchingInfo);
	}
	else
	{
		if (m_DescriptorInfo.pDescriptors != nullptr)
			gcpRendD3D->m_DevBufMan.ReleaseDescriptorBlock(m_DescriptorInfo.pDescriptors);

		m_DescriptorInfo.pDescriptors = gcpRendD3D->m_DevBufMan.CreateDescriptorBlock(descriptorCount);
		m_DescriptorBlock = *m_DescriptorInfo.pDescriptors;

		// gather descriptors for all bound resources and copy to new block. NOTE: CBV_SRV_UAV heap, SMP heap not yet supported
		bBuildSuccess = GatherDescriptors<true, false>(descriptors,
			pDevice->GetResourceDescriptorScratchSpace(),
			&m_ConstantBuffersInUse,
			&m_ShaderResourceViewsInUse,
			&m_UnorderedAccessViewsInUse);
	}

	CRY_ASSERT(descriptors.size() == descriptorCount);
	FillDescriptorBlock(descriptors, m_DescriptorBlock);

	return bBuildSuccess;
}

template<bool bGatherResourcesInUse, bool bGatherPatchingInfo>
bool CDeviceResourceSet_DX12::GatherDescriptors(
  StackDescriptorVector& descriptors,
  CDescriptorBlock& descriptorScratchSpace,
  std::vector<_smart_ptr<CCryDX12Buffer>>* pConstantBuffersInUse,
  std::vector<_smart_ptr<CCryDX12ShaderResourceView>>* pShaderResourceViewsInUse,
  std::vector<_smart_ptr<CCryDX12UnorderedAccessView>>* pUnorderedAccessViewsInUse,
  std::vector<SResourcePatchingInfo>* pTexturePatchingInfo,
  std::vector<SResourcePatchingInfo>* pBufferPatchingInfo,
  std::vector<SConstantBufferPatchingInfo>* pConstantBufferPatchingInfo) const
{
	auto pDevice = GetDevice();

	const int descriptorSize = descriptorScratchSpace.GetDescriptorSize();

	if (bGatherResourcesInUse)
	{
		pConstantBuffersInUse->resize(0);
		pShaderResourceViewsInUse->resize(0);
		pUnorderedAccessViewsInUse->resize(0);

		pConstantBuffersInUse->reserve(m_ConstantBuffers.size());
		pShaderResourceViewsInUse->reserve(m_Textures.size() + m_Buffers.size());
		pUnorderedAccessViewsInUse->reserve(m_Textures.size() + m_Buffers.size());
	}

	bool bValid = true;

	// constant buffers: create descriptors on the fly in scratch buffer
	for (const auto& it : m_ConstantBuffers)
	{
		auto cbData = it.second;
		if (!cbData.resource || !cbData.resource->m_buffer)
		{
			CRY_ASSERT(false && "An invalid constant buffer has been added to the ResourceSet. You must either insert a null-buffer if this was intended, or fix the code that causes this.");
			bValid = false;
			pDevice->GetD3D12Device()->CreateConstantBufferView(nullptr, descriptorScratchSpace.GetHandleOffsetCPU(0));

			descriptors.emplace_back(descriptorScratchSpace.GetHandleOffsetCPU(0));
			descriptorScratchSpace.IncrementCursor();
			continue;
		}

		size_t start, length;
		CCryDX12Buffer* constantBuffer = reinterpret_cast<CCryDX12Buffer*>(cbData.resource->GetD3D(&start, &length));

		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;

			const NCryDX12::CView& pCBV = constantBuffer->GetDX12View();
			assert(pCBV.GetType() == EVT_ConstantBufferView);

			cbvDesc = pCBV.GetCBVDesc();
			cbvDesc.BufferLocation += start;
			cbvDesc.SizeInBytes = length > 0 ? length : cbvDesc.SizeInBytes - start;
			cbvDesc.SizeInBytes = min(cbvDesc.SizeInBytes, D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * NCryDX12::CONSTANT_BUFFER_ELEMENT_SIZE);

			pDevice->GetD3D12Device()->CreateConstantBufferView(&cbvDesc, descriptorScratchSpace.GetHandleOffsetCPU(0));

			if (bGatherPatchingInfo)
			{
				pConstantBufferPatchingInfo->emplace_back();
				SConstantBufferPatchingInfo& patchingInfo = pConstantBufferPatchingInfo->back();

				patchingInfo.desc = cbvDesc;
				patchingInfo.descriptorTableOffset = descriptors.size() * descriptorSize;
				patchingInfo.shaderSlot = it.first;
			}
		}

		descriptors.emplace_back(descriptorScratchSpace.GetHandleOffsetCPU(0));
		descriptorScratchSpace.IncrementCursor();
		if (bGatherResourcesInUse) pConstantBuffersInUse->emplace_back(constantBuffer);
	}

	for (const auto& it : m_Textures)
	{
		CRY_ASSERT(SResourceView::IsShaderResourceView(it.second.view) || SResourceView::IsUnorderedAccessView(it.second.view));

		SResourceView::KeyType srvKey = it.second.view;
		CTexture* pTexture = it.second.resource;

		const bool bBindAsSrv = SResourceView::IsShaderResourceView(srvKey);
		if (!pTexture || !pTexture->GetDevTexture() || !pTexture->GetDevTexture()->GetBaseTexture())
		{
			CRY_ASSERT(false && "An invalid texture has been added to the ResourceSet. You must either insert a null-texture if this was intended, or fix the code that causes this.");
			bValid = false;

			if (bBindAsSrv)
			{
				auto nullDesc = GetNullSrvDesc(pTexture);
				pDevice->GetD3D12Device()->CreateShaderResourceView(nullptr, &nullDesc, descriptorScratchSpace.GetHandleOffsetCPU(0));
			}
			else
			{
				auto nullDesc = GetNullUavDesc(pTexture);
				pDevice->GetD3D12Device()->CreateUnorderedAccessView(nullptr, nullptr, &nullDesc, descriptorScratchSpace.GetHandleOffsetCPU(0));
			}

			descriptors.emplace_back(descriptorScratchSpace.GetHandleOffsetCPU(0));
			descriptorScratchSpace.IncrementCursor();
			continue;
		}

		// add to descriptor block
		if (bBindAsSrv)
		{
			CCryDX12ShaderResourceView* pSrv = GET_DX12_TEXTURE_VIEW(pTexture, srvKey);
			descriptors.emplace_back(pSrv->GetDX12View().GetDescriptorHandle());
			if (bGatherResourcesInUse) pShaderResourceViewsInUse->emplace_back(pSrv);
			if (bGatherPatchingInfo)
			{
				pTexturePatchingInfo->emplace_back();
				SResourcePatchingInfo& patchingInfo = pTexturePatchingInfo->back();

				patchingInfo.descriptorTableOffset = (descriptors.size() - 1) * descriptorSize;
				patchingInfo.shaderSlot = it.first;
				patchingInfo.inUseArrayOffset = (pShaderResourceViewsInUse ? pShaderResourceViewsInUse->size() : 0) - 1;
				patchingInfo.bUnorderedAccess = false;
				patchingInfo.pSrv = pSrv;
			}
		}
		else
		{
			CCryDX12UnorderedAccessView* pUav = GET_DX12_UNORDERED_VIEW(pTexture);
			descriptors.emplace_back(pUav->GetDX12View().GetDescriptorHandle());
			if (bGatherResourcesInUse) pUnorderedAccessViewsInUse->emplace_back(pUav);
			if (bGatherPatchingInfo)
			{
				pTexturePatchingInfo->emplace_back();
				SResourcePatchingInfo& patchingInfo = pTexturePatchingInfo->back();

				patchingInfo.descriptorTableOffset = (descriptors.size() - 1) * descriptorSize;
				patchingInfo.shaderSlot = it.first;
				patchingInfo.inUseArrayOffset = (pUnorderedAccessViewsInUse ? pUnorderedAccessViewsInUse->size() : 0) - 1;
				patchingInfo.bUnorderedAccess = true;
				patchingInfo.pUav = pUav;
			}
		}
	}

	for (const auto& it : m_Buffers)
	{
		auto sbData = it.second;
		SResourceView::KeyType srvKey = sbData.view;
		CGpuBuffer& buffer = sbData.resource;

		const bool bBindAsSrv = SResourceView::IsShaderResourceView(srvKey);
		if (!buffer.GetBuffer() || (bBindAsSrv && !buffer.GetSRV()) || (!bBindAsSrv && !buffer.GetDeviceUAV()))
		{
			CRY_ASSERT(false && "An invalid buffer has been added to the ResourceSet. You must either insert a null-buffer if this was intended, or fix the code that causes this.");
			bValid = false;

			if (bBindAsSrv)
			{
				auto nullDesc = GetNullSrvDesc(buffer);
				pDevice->GetD3D12Device()->CreateShaderResourceView(nullptr, &nullDesc, descriptorScratchSpace.GetHandleOffsetCPU(0));
			}
			else
			{
				auto nullDesc = GetNullUavDesc(buffer);
				pDevice->GetD3D12Device()->CreateUnorderedAccessView(nullptr, nullptr, &nullDesc, descriptorScratchSpace.GetHandleOffsetCPU(0));
			}

			descriptors.emplace_back(descriptorScratchSpace.GetHandleOffsetCPU(0));
			descriptorScratchSpace.IncrementCursor();
			continue;
		}

		// add to descriptor block
		if (bBindAsSrv)
		{
			CCryDX12ShaderResourceView* pSrv = GET_DX12_BUFFER_VIEW(sbData.resource);
			descriptors.emplace_back(pSrv->GetDX12View().GetDescriptorHandle());
			if (bGatherResourcesInUse) pShaderResourceViewsInUse->emplace_back(pSrv);
			if (bGatherPatchingInfo)
			{
				pBufferPatchingInfo->emplace_back();
				SResourcePatchingInfo& patchingInfo = pBufferPatchingInfo->back();

				patchingInfo.descriptorTableOffset = (descriptors.size() - 1) * descriptorSize;
				patchingInfo.shaderSlot = it.first;
				patchingInfo.inUseArrayOffset = (pShaderResourceViewsInUse ? pShaderResourceViewsInUse->size() : 0) - 1;
				patchingInfo.bUnorderedAccess = false;
				patchingInfo.pSrv = pSrv;
			}
		}
		else
		{
			CCryDX12UnorderedAccessView* pUav = GET_DX12_UNORDERED_VIEW(&sbData.resource);
			descriptors.emplace_back(pUav->GetDX12View().GetDescriptorHandle());
			if (bGatherResourcesInUse) pUnorderedAccessViewsInUse->emplace_back(pUav);
			if (bGatherPatchingInfo)
			{
				pBufferPatchingInfo->emplace_back();
				SResourcePatchingInfo& patchingInfo = pBufferPatchingInfo->back();

				patchingInfo.descriptorTableOffset = (descriptors.size() - 1) * descriptorSize;
				patchingInfo.shaderSlot = it.first;
				patchingInfo.inUseArrayOffset = (pUnorderedAccessViewsInUse ? pUnorderedAccessViewsInUse->size() : 0) - 1;
				patchingInfo.bUnorderedAccess = true;
				patchingInfo.pUav = pUav;
			}
		}
	}

	return bValid;
}

static const UINT DescriptorRangesOfOnes[256] = {
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
};

void CDeviceResourceSet_DX12::FillDescriptorBlock(
	StackDescriptorVector& descriptors,
	CDescriptorBlock& descriptorBlock) const
{
	CRY_ASSERT(descriptorBlock.GetCapacity() - descriptorBlock.GetCursor() == descriptors.size());
	auto pDevice = GetDevice()->GetD3D12Device();

	if (!descriptors.empty())
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = descriptorBlock.GetHandleOffsetCPU(0);

		// Deal with unbound number of descriptors in chunks of 256
		for (UINT i = 0, srcSize = UINT(descriptors.size()), chunkSize = CRY_ARRAY_COUNT(DescriptorRangesOfOnes); i < srcSize; i += chunkSize)
		{
			const UINT dstRangeCount = 1;
			const UINT srcRangeCount = std::min(srcSize - i, chunkSize);

			pDevice->CopyDescriptors(
				dstRangeCount, &dstHandle, &srcRangeCount,
				srcRangeCount, &descriptors[i], &DescriptorRangesOfOnes[0],
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}
}

void CDeviceResourceSet_DX12::PatchDescriptors(CDescriptorBlock& descriptorScratchSpace)
{
	CRY_ASSERT((m_Flags & EFlags_Multibuffered) != 0 && m_DescriptorInfo.pDescriptorSet);
	auto pDevice = GetDevice()->GetD3D12Device();
	auto pDescriptors = m_DescriptorInfo.pDescriptorSet;

	size_t dstDescriptorCount = pDescriptors->texturePatchingInfo.size() + pDescriptors->bufferPatchingInfo.size() + pDescriptors->constantBufferPatchingInfo.size();
	size_t srcDescriptorCount = dstDescriptorCount;

	CryStackAllocatorWithSizeVector(D3D12_CPU_DESCRIPTOR_HANDLE, dstDescriptorCount, dstDescriptorMem, NoAlign);
	CryStackAllocatorWithSizeVector(D3D12_CPU_DESCRIPTOR_HANDLE, srcDescriptorCount, srcDescriptorMem, NoAlign);
	StackDescriptorVector dstDescriptors(dstDescriptorMem);
	StackDescriptorVector srcDescriptors(srcDescriptorMem);
	dstDescriptors.reserve(dstDescriptorCount);
	srcDescriptors.reserve(srcDescriptorCount);

	for (auto& patchingInfo : pDescriptors->constantBufferPatchingInfo)
	{
		auto it = m_ConstantBuffers.find(patchingInfo.shaderSlot);
		CRY_ASSERT(it != m_ConstantBuffers.end());

		auto cbData = it->second;
		CRY_ASSERT(cbData.resource && cbData.resource->m_buffer);

		size_t start, length;
		CCryDX12Buffer* constantBuffer = reinterpret_cast<CCryDX12Buffer*>(cbData.resource->GetD3D(&start, &length));

		const NCryDX12::CView& pCBV = constantBuffer->GetDX12View();
		assert(pCBV.GetType() == EVT_ConstantBufferView);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = pCBV.GetCBVDesc();
		cbvDesc.BufferLocation += start;
		cbvDesc.SizeInBytes = length > 0 ? length : cbvDesc.SizeInBytes - start;
		cbvDesc.SizeInBytes = min(cbvDesc.SizeInBytes, D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * NCryDX12::CONSTANT_BUFFER_ELEMENT_SIZE);

		if (cbvDesc.BufferLocation != patchingInfo.desc.BufferLocation || cbvDesc.SizeInBytes != patchingInfo.desc.SizeInBytes)
		{
			pDevice->CreateConstantBufferView(&cbvDesc, descriptorScratchSpace.GetHandleOffsetCPU(0));
			patchingInfo.desc = cbvDesc; // update patching info

			srcDescriptors.emplace_back(descriptorScratchSpace.GetHandleOffsetCPU(0));
			D3D12_CPU_DESCRIPTOR_HANDLE dstOffset = { patchingInfo.descriptorTableOffset };
			dstDescriptors.emplace_back(dstOffset);

			descriptorScratchSpace.IncrementCursor();
		}
	}

	for (auto& patchingInfo : pDescriptors->bufferPatchingInfo)
	{
		auto it = m_Buffers.find(patchingInfo.shaderSlot);
		CRY_ASSERT(it != m_Buffers.end());

		auto& buffer = it->second.resource;
		if (patchingInfo.bUnorderedAccess)
		{
			CCryDX12UnorderedAccessView* pUav = GET_DX12_UNORDERED_VIEW(&buffer);
			CRY_ASSERT(pUav != nullptr);

			if (pUav != patchingInfo.pUav)
			{
				srcDescriptors.emplace_back(pUav->GetDX12View().GetDescriptorHandle());
				patchingInfo.pUav = pUav; // update patching info

				D3D12_CPU_DESCRIPTOR_HANDLE dstOffset = { patchingInfo.descriptorTableOffset };
				dstDescriptors.emplace_back(dstOffset);

				CRY_ASSERT(patchingInfo.inUseArrayOffset >= 0 && patchingInfo.inUseArrayOffset < m_UnorderedAccessViewsInUse.size());
				m_UnorderedAccessViewsInUse[patchingInfo.inUseArrayOffset] = pUav;
			}
		}
		else
		{
			CCryDX12ShaderResourceView* pSrv = GET_DX12_BUFFER_VIEW(buffer);
			CRY_ASSERT(pSrv != nullptr);

			if (pSrv != patchingInfo.pSrv)
			{
				srcDescriptors.emplace_back(pSrv->GetDX12View().GetDescriptorHandle());
				patchingInfo.pSrv = pSrv;

				D3D12_CPU_DESCRIPTOR_HANDLE dstOffset = { patchingInfo.descriptorTableOffset };
				dstDescriptors.emplace_back(dstOffset);

				CRY_ASSERT(patchingInfo.inUseArrayOffset >= 0 && patchingInfo.inUseArrayOffset < m_ShaderResourceViewsInUse.size());
				m_ShaderResourceViewsInUse[patchingInfo.inUseArrayOffset] = pSrv;
			}
		}
	}

	for (auto& patchingInfo : pDescriptors->texturePatchingInfo)
	{
		auto it = m_Textures.find(patchingInfo.shaderSlot);
		CRY_ASSERT(it != m_Textures.end());

		SResourceView::KeyType srvKey = it->second.view;
		auto& texture = it->second.resource;
		if (patchingInfo.bUnorderedAccess)
		{
			CCryDX12UnorderedAccessView* pUav = GET_DX12_UNORDERED_VIEW(texture);
			CRY_ASSERT(pUav != nullptr);

			if (pUav != patchingInfo.pUav)
			{
				srcDescriptors.emplace_back(pUav->GetDX12View().GetDescriptorHandle());
				patchingInfo.pUav = pUav; // update patching info

				D3D12_CPU_DESCRIPTOR_HANDLE dstOffset = { patchingInfo.descriptorTableOffset };
				dstDescriptors.emplace_back(dstOffset);

				CRY_ASSERT(patchingInfo.inUseArrayOffset >= 0 && patchingInfo.inUseArrayOffset < m_UnorderedAccessViewsInUse.size());
				m_UnorderedAccessViewsInUse[patchingInfo.inUseArrayOffset] = pUav;
			}
		}
		else
		{
			CCryDX12ShaderResourceView* pSrv = GET_DX12_TEXTURE_VIEW(texture, srvKey);
			CRY_ASSERT(pSrv != nullptr);

			if (pSrv != patchingInfo.pSrv)
			{
				srcDescriptors.emplace_back(pSrv->GetDX12View().GetDescriptorHandle());
				patchingInfo.pSrv = pSrv;

				D3D12_CPU_DESCRIPTOR_HANDLE dstOffset = { patchingInfo.descriptorTableOffset };
				dstDescriptors.emplace_back(dstOffset);

				CRY_ASSERT(patchingInfo.inUseArrayOffset >= 0 && patchingInfo.inUseArrayOffset < m_ShaderResourceViewsInUse.size());
				m_ShaderResourceViewsInUse[patchingInfo.inUseArrayOffset] = pSrv;
			}
		}
	}

	if (!srcDescriptors.empty())
	{
		auto& allocator = pDescriptors->descriptorBlockAllocator;
		allocator.Release(pDescriptors->pCurrentDescriptorBlock);

		bool bNewBlock = false;
		pDescriptors->pCurrentDescriptorBlock = allocator.Allocate(&bNewBlock);
		m_DescriptorBlock = CDescriptorBlock(*pDescriptors->pCurrentDescriptorBlock);

		if (bNewBlock)
		{
			const int descriptorCount = m_ConstantBuffers.size() + m_Textures.size() + m_Buffers.size();

			CryStackAllocatorWithSizeVector(D3D12_CPU_DESCRIPTOR_HANDLE, descriptorCount, descriptorMem, NoAlign);
			StackDescriptorVector descriptors(descriptorMem);
			descriptors.reserve(descriptorCount);

			GatherDescriptors<false, false>(descriptors, descriptorScratchSpace);
			CRY_ASSERT(descriptors.size() == descriptorCount);
			FillDescriptorBlock(descriptors, m_DescriptorBlock);
		}
		else
		{
			// patch base address of new block
			const SIZE_T offset = m_DescriptorBlock.GetHandleOffsetCPU(0).ptr;
			for (auto& descriptor : dstDescriptors)
				descriptor.ptr += offset;

			// Deal with unbound number of descriptors in chunks of 256
			for (UINT i = 0,
				srcSize = UINT(srcDescriptors.size()),
				dstSize = UINT(srcDescriptors.size()),
				chunkSize = CRY_ARRAY_COUNT(DescriptorRangesOfOnes); i < srcSize; i += chunkSize)
			{
				pDevice->CopyDescriptors(
					std::min(srcSize - i, chunkSize), &dstDescriptors[i], &DescriptorRangesOfOnes[0],
					std::min(dstSize - i, chunkSize), &srcDescriptors[i], &DescriptorRangesOfOnes[0],
					D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}
		}
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceResourceLayout_DX12 : public CDeviceResourceLayout
{
public:
	CDeviceResourceLayout_DX12(CDevice* pDevice, UsedBindSlotSet requiredResourceBindings)
		: CDeviceResourceLayout(requiredResourceBindings)
		, m_pDevice(pDevice)
	{}

	bool            Init(const SDeviceResourceLayoutDesc& desc);
	CRootSignature* GetRootSignature() const { return m_pRootSignature.get(); }

protected:
	DX12_PTR(CDevice) m_pDevice;
	DX12_PTR(CRootSignature) m_pRootSignature;
};

bool CDeviceResourceLayout_DX12::Init(const SDeviceResourceLayoutDesc& desc)
{
	// *INDENT-OFF* - code formatter doesn't work so well on lambdas
	auto getShaderVisibility = [&](::EShaderStage shaderStages)
	{
		D3D12_SHADER_VISIBILITY shaderVisibility[eHWSC_Num + 1] =
		{
			D3D12_SHADER_VISIBILITY_VERTEX,     // eHWSC_Vertex
			D3D12_SHADER_VISIBILITY_PIXEL,      // eHWSC_Pixel
			D3D12_SHADER_VISIBILITY_GEOMETRY,   // eHWSC_Geometry
			D3D12_SHADER_VISIBILITY_ALL,        // eHWSC_Compute
			D3D12_SHADER_VISIBILITY_DOMAIN,     // eHWSC_Domain
			D3D12_SHADER_VISIBILITY_HULL,       // eHWSC_Hull
			D3D12_SHADER_VISIBILITY_ALL,        // eHWSC_Num
		};

		EHWShaderClass shaderClass = eHWSC_Num;

		if ((int(shaderStages) & (int(shaderStages)-1)) == 0) // only bound to a single shader stage?
		{
			for (shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass+1))
			{
				if (shaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
					break;
			}
		}

		return shaderVisibility[shaderClass];
	};
	// *INDENT-ON*

	SResourceMappings resourceMappings;

	// inline constants
	if (desc.m_InlineConstantCount > 0)
	{
		resourceMappings.m_RootParameters[0].InitAsConstants(desc.m_InlineConstantCount, InlineConstantsShaderSlot);

		++resourceMappings.m_NumRootParameters;

		assert(resourceMappings.m_NumRootParameters <= 64);
	}

	// inline constant buffers
	for (auto it : desc.m_ConstantBuffers)
	{
		const int bindSlot = it.first;
		const SDeviceResourceLayoutDesc::SInlineConstantBuffer& cb = it.second;

		resourceMappings.m_RootParameters[bindSlot].InitAsConstantBufferView(cb.shaderSlot, 0, getShaderVisibility(cb.shaderStages));

		++resourceMappings.m_NumRootParameters;

		assert(resourceMappings.m_NumRootParameters <= 64);
	}

	// descriptor table resource sets
	for (auto itRS : desc.m_ResourceSets)
	{
		const int bindSlot = itRS.first;
		CDeviceResourceSet* pResourceSet = itRS.second.get();
		auto shaderVisibility = getShaderVisibility(pResourceSet->GetShaderStages());

		uint32 startDesc = resourceMappings.m_DescRangeCursor;
		uint32 tableOffset = 0;

		for (auto itCB : pResourceSet->m_ConstantBuffers)
		{
			const DeviceResourceBinding::SShaderSlot slot = itCB.first;

			CD3DX12_DESCRIPTOR_RANGE cbDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, slot.slotNumber, 0, tableOffset);
			resourceMappings.m_DescRanges[resourceMappings.m_DescRangeCursor] = cbDescRange;

			++resourceMappings.m_DescRangeCursor;
			++tableOffset;

			assert(resourceMappings.m_DescRangeCursor <= 128);
		}

		for (auto itTex : pResourceSet->m_Textures)
		{
			const DeviceResourceBinding::SShaderSlot slot = itTex.first;
			SResourceView::KeyType viewKey = itTex.second.view;

			auto rangeType = SResourceView::IsShaderResourceView(viewKey) ? D3D12_DESCRIPTOR_RANGE_TYPE_SRV : D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			CD3DX12_DESCRIPTOR_RANGE texDescRange(rangeType, 1, slot.slotNumber, 0, tableOffset);
			resourceMappings.m_DescRanges[resourceMappings.m_DescRangeCursor] = texDescRange;

			++resourceMappings.m_DescRangeCursor;
			++tableOffset;

			assert(resourceMappings.m_DescRangeCursor <= 128);
		}

		for (auto itBuffer : pResourceSet->m_Buffers)
		{
			const DeviceResourceBinding::SShaderSlot slot = itBuffer.first;
			SResourceView::KeyType viewKey = itBuffer.second.view;

			auto rangeType = SResourceView::IsShaderResourceView(viewKey) ? D3D12_DESCRIPTOR_RANGE_TYPE_SRV : D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			CD3DX12_DESCRIPTOR_RANGE bufferDescRange(rangeType, 1, slot.slotNumber, 0, tableOffset);
			resourceMappings.m_DescRanges[resourceMappings.m_DescRangeCursor] = bufferDescRange;

			++resourceMappings.m_DescRangeCursor;
			++tableOffset;

			assert(resourceMappings.m_DescRangeCursor <= 128);
		}

		if ((resourceMappings.m_DescRangeCursor - startDesc) > 0)
		{
			resourceMappings.m_RootParameters[bindSlot].InitAsDescriptorTable(
			  resourceMappings.m_DescRangeCursor - startDesc, &resourceMappings.m_DescRanges[startDesc], shaderVisibility);

			++resourceMappings.m_NumRootParameters;

			assert(resourceMappings.m_NumRootParameters <= 64);
		}

		for (auto itSampler : pResourceSet->m_Samplers)
		{
			const DeviceResourceBinding::SShaderSlot slot = itSampler.first;
			const CDeviceResourceSet::SSamplerData& samplerData = itSampler.second;

			const NCryDX12::CSamplerState& State = GET_DX12_SAMPLER(CTexture::s_TexStates[samplerData.resource])->GetDX12SamplerState();
			auto samplerDesc = State.GetSamplerDesc();

			// copy parameters from sampler desc first
			memcpy(&resourceMappings.m_StaticSamplers[resourceMappings.m_NumStaticSamplers], &samplerDesc, sizeof(samplerDesc));

			// fill in remaining parameters
			auto& staticSamplerDesc = resourceMappings.m_StaticSamplers[resourceMappings.m_NumStaticSamplers];
			staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
			staticSamplerDesc.MinLOD = samplerDesc.MinLOD;
			staticSamplerDesc.MaxLOD = samplerDesc.MaxLOD;
			staticSamplerDesc.ShaderRegister = slot.slotNumber;
			staticSamplerDesc.RegisterSpace = 0;
			staticSamplerDesc.ShaderVisibility = shaderVisibility;

			++resourceMappings.m_NumStaticSamplers;

			assert(resourceMappings.m_NumStaticSamplers <= 16);
		}
	}

	m_pDevice->GetRootSignatureCache().GetOrCreateRootSignature(resourceMappings, true, m_pDevice->GetNodeMask(), m_pRootSignature);
	return m_pRootSignature != nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceGraphicsPSO_DX12 : public CDeviceGraphicsPSO
{
public:

	CDeviceGraphicsPSO_DX12(CDevice* pDevice)
		: m_pDevice(pDevice)
	{}

	virtual bool             Init(const CDeviceGraphicsPSODesc& desc) final;

	CGraphicsPSO*            GetGraphicsPSO() const       { return m_pGraphicsPSO.get(); }
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }

protected:
	SOnDemandD3DVertexDeclaration m_InputLayout;
	D3D12_PRIMITIVE_TOPOLOGY      m_PrimitiveTopology;

	DX12_PTR(CDevice) m_pDevice;
	DX12_PTR(CGraphicsPSO) m_pGraphicsPSO;
};

bool CDeviceGraphicsPSO_DX12::Init(const CDeviceGraphicsPSODesc& psoDesc)
{
	m_bValid = false;
	m_nUpdateCount++;

	if (psoDesc.m_pResourceLayout == NULL)
		return false;

	auto hwShaders = SDeviceObjectHelpers::GetShaderInstanceInfo(psoDesc.m_pShader, psoDesc.m_technique, psoDesc.m_ShaderFlags_RT, psoDesc.m_ShaderFlags_MD, psoDesc.m_ShaderFlags_MDV, nullptr, psoDesc.m_bAllowTesselation);
	// validate shaders first
	for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass < eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
	{
		if (hwShaders[shaderClass].pHwShader && hwShaders[shaderClass].pHwShaderInstance == NULL)
			return false;

		// TODO: remove
		m_pHwShaders[shaderClass] = hwShaders[shaderClass].pHwShader;
		m_pHwShaderInstances[shaderClass] = hwShaders[shaderClass].pHwShaderInstance;
	}

	D3D11_RASTERIZER_DESC rasterizerDesc;
	D3D11_BLEND_DESC blendDesc;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	psoDesc.FillDescs(rasterizerDesc, blendDesc, depthStencilDesc);

	// prepare pso init params
	CGraphicsPSO::SInitParams psoInitParams;
	ZeroStruct(psoInitParams);

	// root signature
	psoInitParams.m_Desc.pRootSignature = reinterpret_cast<CDeviceResourceLayout_DX12*>(psoDesc.m_pResourceLayout)->GetRootSignature()->GetD3D12RootSignature();

	// blend state
	{
		psoInitParams.m_Desc.BlendState.AlphaToCoverageEnable = blendDesc.AlphaToCoverageEnable;
		psoInitParams.m_Desc.BlendState.IndependentBlendEnable = blendDesc.IndependentBlendEnable;

		for (int i = 0; i < CRY_ARRAY_COUNT(blendDesc.RenderTarget); ++i)
		{
			uint32 RenderTargetWriteMask = i < psoDesc.m_RenderTargetFormats.size() ? CTexture::WriteMaskFromTexFormat(psoDesc.m_RenderTargetFormats[i]) : 0;

			psoInitParams.m_Desc.BlendState.RenderTarget[i].BlendEnable = blendDesc.RenderTarget[i].BlendEnable;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].LogicOpEnable = false;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].SrcBlend = (D3D12_BLEND)blendDesc.RenderTarget[i].SrcBlend;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].DestBlend = (D3D12_BLEND)blendDesc.RenderTarget[i].DestBlend;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].BlendOp = (D3D12_BLEND_OP)blendDesc.RenderTarget[i].BlendOp;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].SrcBlendAlpha = (D3D12_BLEND)blendDesc.RenderTarget[i].SrcBlendAlpha;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].DestBlendAlpha = (D3D12_BLEND)blendDesc.RenderTarget[i].DestBlendAlpha;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].BlendOpAlpha = (D3D12_BLEND_OP)blendDesc.RenderTarget[i].BlendOpAlpha;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_CLEAR;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].RenderTargetWriteMask = blendDesc.RenderTarget[i].RenderTargetWriteMask & RenderTargetWriteMask;
		}
	}

	// depth stencil and rasterizer state
	memcpy(&psoInitParams.m_Desc.DepthStencilState, &depthStencilDesc, sizeof(depthStencilDesc));
	memcpy(&psoInitParams.m_Desc.RasterizerState, &rasterizerDesc, sizeof(rasterizerDesc));

	auto extractShaderBytecode = [&](EHWShaderClass shaderClass)
	{
		return hwShaders[shaderClass].pHwShader
		       ? reinterpret_cast<CCryDX12Shader*>(hwShaders[shaderClass].pDeviceShader)->GetD3D12ShaderBytecode()
		       : g_EmptyShader;
	};

	psoInitParams.m_Desc.NodeMask = m_pDevice->GetNodeMask();
	psoInitParams.m_Desc.VS = extractShaderBytecode(eHWSC_Vertex);
	psoInitParams.m_Desc.DS = extractShaderBytecode(eHWSC_Domain);
	psoInitParams.m_Desc.HS = extractShaderBytecode(eHWSC_Hull);
	psoInitParams.m_Desc.GS = extractShaderBytecode(eHWSC_Geometry);
	psoInitParams.m_Desc.PS = extractShaderBytecode(eHWSC_Pixel);

	psoInitParams.m_Desc.SampleMask = UINT_MAX;
	psoInitParams.m_Desc.SampleDesc.Count = 1;
	// primitive topology
	{
		m_PrimitiveTopology = gcpRendD3D->FX_ConvertPrimitiveType(psoDesc.m_PrimitiveType);

		struct
		{
			ERenderPrimitiveType          primitiveType;
			D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology;
		}
		topologyTypes[] =
		{
			{ eptUnknown,                D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED },
			{ eptTriangleList,           D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE  },
			{ eptTriangleStrip,          D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE  },
			{ eptLineList,               D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE      },
			{ eptLineStrip,              D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE      },
			{ eptPointList,              D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT     },
			{ ept1ControlPointPatchList, D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH     },
			{ ept2ControlPointPatchList, D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH     },
			{ ept3ControlPointPatchList, D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH     },
			{ ept4ControlPointPatchList, D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH     },
		};

		psoInitParams.m_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
		for (int i = 0; i < CRY_ARRAY_COUNT(topologyTypes); ++i)
		{
			if (topologyTypes[i].primitiveType == psoDesc.m_PrimitiveType)
			{
				psoInitParams.m_Desc.PrimitiveTopologyType = topologyTypes[i].primitiveTopology;
				break;
			}
		}
	}

	// input layout
	m_InputLayout.m_Declaration.SetUse(0);
	if (psoDesc.m_VertexFormat != eVF_Unknown)
	{
		if (CHWShader_D3D::SHWSInstance* pVsInstance = reinterpret_cast<CHWShader_D3D::SHWSInstance*>(hwShaders[eHWSC_Vertex].pHwShaderInstance))
		{
			uint8 streamMask = psoDesc.CombineVertexStreamMasks(uint8(pVsInstance->m_VStreamMask_Decl), psoDesc.m_ObjectStreamMask);

			const bool bMorph = false;
			const bool bInstanced = (streamMask & VSM_INSTANCED) != 0;

			gcpRendD3D->m_RP.OnDemandVertexDeclaration(m_InputLayout, streamMask >> 1, psoDesc.m_VertexFormat, bMorph, bInstanced);

			// D3D12_INPUT_ELEMENT_DESC := D3D11_INPUT_ELEMENT_DESC
			psoInitParams.m_Desc.InputLayout.pInputElementDescs = reinterpret_cast<D3D12_INPUT_ELEMENT_DESC*>(&m_InputLayout.m_Declaration[0]);
			psoInitParams.m_Desc.InputLayout.NumElements = m_InputLayout.m_Declaration.size();
		}
	}

	// render targets
	for (int i = 0; i < psoDesc.m_RenderTargetFormats.size(); ++i)
	{
		psoInitParams.m_Desc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;

		if (psoDesc.m_RenderTargetFormats[i] != eTF_Unknown)
		{
			psoInitParams.m_Desc.RTVFormats[i] = CTexture::DeviceFormatFromTexFormat(psoDesc.m_RenderTargetFormats[i]);
			psoInitParams.m_Desc.NumRenderTargets = i + 1;
		}
	}

	if (psoDesc.m_DepthStencilFormat != eTF_Unknown)
		psoInitParams.m_Desc.DSVFormat = CTexture::DeviceFormatFromTexFormat(psoDesc.m_DepthStencilFormat);

	#if defined(ENABLE_PROFILING_CODE)
	m_PrimitiveTypeForProfiling = psoDesc.m_PrimitiveType;
	#endif

	m_pDevice->GetPSOCache().GetOrCreatePSO(psoInitParams, m_pGraphicsPSO);

	return (m_bValid = (m_pGraphicsPSO != nullptr));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceComputePSO_DX12 : public CDeviceComputePSO
{
public:

	CDeviceComputePSO_DX12(CDevice* pDevice)
		: m_pDevice(pDevice)
	{}

	virtual bool Init(const CDeviceComputePSODesc& desc) final;

	CComputePSO* GetComputePSO() const { return m_pComputePSO.get(); }

protected:

	DX12_PTR(CDevice) m_pDevice;
	DX12_PTR(CComputePSO) m_pComputePSO;
};

bool CDeviceComputePSO_DX12::Init(const CDeviceComputePSODesc& psoDesc)
{
	m_bValid = false;

	if (psoDesc.m_pResourceLayout == NULL)
		return false;

	auto hwShaders = SDeviceObjectHelpers::GetShaderInstanceInfo(psoDesc.m_pShader, psoDesc.m_technique, psoDesc.m_ShaderFlags_RT, psoDesc.m_ShaderFlags_MD, psoDesc.m_ShaderFlags_MDV, nullptr, false);

	// validate shaders first
	if (hwShaders[eHWSC_Compute].pHwShader && hwShaders[eHWSC_Compute].pHwShaderInstance == NULL)
		return false;

	// TODO: remove
	m_pHwShader = hwShaders[eHWSC_Compute].pHwShader;
	m_pHwShaderInstance = hwShaders[eHWSC_Compute].pHwShaderInstance;

	// prepare pso init params
	CComputePSO::SInitParams psoInitParams;
	ZeroStruct(psoInitParams);

	// root signature
	psoInitParams.m_Desc.pRootSignature = reinterpret_cast<CDeviceResourceLayout_DX12*>(psoDesc.m_pResourceLayout)->GetRootSignature()->GetD3D12RootSignature();

	// *INDENT-OFF*
	auto extractShaderBytecode = [&](EHWShaderClass shaderClass)
	{
		return hwShaders[shaderClass].pHwShader
			? reinterpret_cast<CCryDX12Shader*>(hwShaders[shaderClass].pDeviceShader)->GetD3D12ShaderBytecode()
			: g_EmptyShader;
	};
	// *INDENT-ON*

	psoInitParams.m_Desc.NodeMask = m_pDevice->GetNodeMask();
	psoInitParams.m_Desc.CS = extractShaderBytecode(eHWSC_Compute);

	m_pDevice->GetPSOCache().GetOrCreatePSO(psoInitParams, m_pComputePSO);

	return (m_bValid = (m_pComputePSO != nullptr));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CDeviceTimestampGroup::s_reservedGroups[4] = { false, false, false, false };

CDeviceTimestampGroup::CDeviceTimestampGroup()
	: m_numTimestamps(0)
	, m_groupIndex(0xFFFFFFFF)
	, m_fence(0)
	, m_frequency(0)
{
	m_timeValues.fill(0);
}

CDeviceTimestampGroup::~CDeviceTimestampGroup()
{
	s_reservedGroups[m_groupIndex] = false;
}

void CDeviceTimestampGroup::Init()
{
	gcpRendD3D->m_DevMan.CreateFence(m_fence);
	m_numTimestamps = 0;

	m_groupIndex = 0xFFFFFFFF;
	for (uint32 i = 0; i < CRY_ARRAY_COUNT(s_reservedGroups); i++)
	{
		if (s_reservedGroups[i] == false)
		{
			s_reservedGroups[i] = true;
			m_groupIndex = i;
			break;
		}
	}

	assert(m_groupIndex < 0xFFFFFFFF);
}

void CDeviceTimestampGroup::BeginMeasurement()
{
	m_numTimestamps = 0;
}

void CDeviceTimestampGroup::EndMeasurement()
{
	gcpRendD3D->m_DevMan.IssueFence(m_fence);

	CCryDX12DeviceContext* m_pDeviceContext = (CCryDX12DeviceContext*)gcpRendD3D->GetDeviceContext().GetRealDeviceContext();
	m_frequency = m_pDeviceContext->GetTimestampFrequency();
}

uint32 CDeviceTimestampGroup::IssueTimestamp()
{
	assert(m_numTimestamps < kMaxTimestamps);

	uint32 timestampIndex = m_groupIndex * kMaxTimestamps + m_numTimestamps;

	CCryDX12DeviceContext* m_pDeviceContext = (CCryDX12DeviceContext*)gcpRendD3D->GetDeviceContext().GetRealDeviceContext();
	m_pDeviceContext->InsertTimestamp(timestampIndex, 0);

	++m_numTimestamps;
	return timestampIndex;
}

bool CDeviceTimestampGroup::ResolveTimestamps()
{
	if (m_numTimestamps == 0)
		return true;

	if (gcpRendD3D->m_DevMan.SyncFence(m_fence, false, true) != S_OK)
		return false;

	CCryDX12DeviceContext* m_pDeviceContext = (CCryDX12DeviceContext*)gcpRendD3D->GetDeviceContext().GetRealDeviceContext();
	m_pDeviceContext->ResolveTimestamps();
	m_pDeviceContext->QueryTimestamps(m_groupIndex * kMaxTimestamps, m_numTimestamps, &m_timeValues[0]);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeviceCommandListImpl::SetProfilerMarker(const char* label)
{
#if defined(ENABLE_FRAME_PROFILER_LABELS)
	PIXSetMarker(m_sharedState.pCommandList->GetD3D12CommandList(), 0, label);
#endif
}

void CDeviceCommandListImpl::BeginProfilerEvent(const char* label)
{
#if defined(ENABLE_FRAME_PROFILER_LABELS)
	PIXBeginEvent(m_sharedState.pCommandList->GetD3D12CommandList(), 0, label);
#endif
}

void CDeviceCommandListImpl::EndProfilerEvent(const char* label)
{
#if defined(ENABLE_FRAME_PROFILER_LABELS)
	PIXEndEvent(m_sharedState.pCommandList->GetD3D12CommandList());
#endif
}

void CDeviceCommandListImpl::ResetImpl()
{
	m_graphicsState.custom.primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

void CDeviceCommandListImpl::LockToThreadImpl()
{
	NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;
	pCommandListDX12->Begin();
	pCommandListDX12->SetResourceAndSamplerStateHeaps();
}

void CDeviceCommandListImpl::CloseImpl()
{
	FUNCTION_PROFILER_RENDERER
	m_sharedState.pCommandList->End();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeviceGraphicsCommandInterfaceImpl::PrepareUAVsForUseImpl(uint32 viewCount, CGpuBuffer** pViews) const
{
	NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;

	for (int v = 0; v < viewCount; ++v)
	{
		const NCryDX12::CView& View = reinterpret_cast<CCryDX12UnorderedAccessView*>(pViews[v]->GetDeviceUAV())->GetDX12View();
		NCryDX12::CResource& Resource = View.GetDX12Resource();
		const D3D12_RESOURCE_STATES desiredState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		if (Resource.InitHasBeenDeferred())
		{
			Resource.InitDeferred(pCommandListDX12, desiredState);
		}

		assert(!((pCommandListDX12->GetD3D12ListType() != D3D12_COMMAND_LIST_TYPE_DIRECT) && (Resource.GetTargetState() & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) && (!Resource.IsOffCard())));

		pCommandListDX12->PrepareResourceUAVUsage(Resource, View, desiredState);
	}
}

void CDeviceGraphicsCommandInterfaceImpl::PrepareRenderTargetsForUseImpl(uint32 targetCount, CTexture** pTargets, SDepthTexture* pDepthTarget, const SResourceView::KeyType* pRenderTargetViews /*= nullptr*/) const
{
	NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;

	// Get current depth stencil views
	if (pDepthTarget)
	{
		const NCryDX12::CView& View = GET_DX12_DEPTHSTENCIL_VIEW(pDepthTarget)->GetDX12View();
		NCryDX12::CResource& Resource = View.GetDX12Resource();
		const D3D12_RESOURCE_STATES desiredState = Resource.GetDesc().Flags & D3D11_DSV_READ_ONLY_DEPTH ? D3D12_RESOURCE_STATE_DEPTH_READ : D3D12_RESOURCE_STATE_DEPTH_WRITE;

		pCommandListDX12->PrepareResourceDSVUsage(Resource, View, desiredState);
	}

	// Get current render target views
	for (int i = 0; i < targetCount; ++i)
	{
		if (pTargets[i])
		{
			const bool bHasCustomView = pRenderTargetViews && pRenderTargetViews[i] != SResourceView::DefaultRendertargetView;
			const SResourceView::KeyType rtvKey = bHasCustomView ? pRenderTargetViews[i] : SResourceView::RenderTargetView(pTargets[i]->GetDstFormat()).m_Desc.Key;
			CRY_ASSERT(SResourceView::IsRenderTargetView(rtvKey));

			const NCryDX12::CView& UnpatchedView = GET_DX12_RENDERTARGET_VIEW(pTargets[i], rtvKey)->GetDX12View();
			const NCryDX12::CView& View = pCommandListDX12->PatchRenderTargetView(UnpatchedView);
			NCryDX12::CResource& Resource = View.GetDX12Resource();
			const D3D12_RESOURCE_STATES desiredState = D3D12_RESOURCE_STATE_RENDER_TARGET;

			pCommandListDX12->PrepareResourceRTVUsage(Resource, View, desiredState);
		}
	}
}

void CDeviceGraphicsCommandInterfaceImpl::PrepareResourcesForUseImpl(uint32 bindSlot, CDeviceResourceSet* pResources, ::EShaderStage srvUsage) const
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);

	NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;
	CDeviceResourceSet_DX12* pResourcesDX12 = reinterpret_cast<CDeviceResourceSet_DX12*>(pResources);

	if (pResourcesDX12->m_Flags & CDeviceResourceSet::EFlags_Multibuffered)
	{
		pResourcesDX12->PatchDescriptors(GetDevice()->GetResourceDescriptorScratchSpace());
	}

	for (auto& it : pResourcesDX12->m_ConstantBuffersInUse)
	{
		NCryDX12::CResource& Resource = it->GetDX12Resource();
		const D3D12_RESOURCE_STATES desiredState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

		if (Resource.InitHasBeenDeferred())
		{
			Resource.InitDeferred(pCommandListDX12, desiredState);
		}

		pCommandListDX12->PrepareResourceCBVUsage(Resource, desiredState);
	}

	for (auto& it : pResourcesDX12->m_ShaderResourceViewsInUse)
	{
		const NCryDX12::CView& View = it->GetDX12View();
		NCryDX12::CResource& Resource = View.GetDX12Resource();
		const D3D12_RESOURCE_STATES desiredState =
			(srvUsage & EShaderStage_Pixel ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_COMMON) |
			(srvUsage & EShaderStage_Compute ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_COMMON);

		if (Resource.InitHasBeenDeferred())
		{
			Resource.InitDeferred(pCommandListDX12, desiredState);
		}

		assert(!((pCommandListDX12->GetD3D12ListType() != D3D12_COMMAND_LIST_TYPE_DIRECT) && (Resource.GetTargetState() & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) && (!Resource.IsOffCard())));

		pCommandListDX12->PrepareResourceSRVUsage(Resource, View, desiredState);
	}

	for (auto& it : pResourcesDX12->m_UnorderedAccessViewsInUse)
	{
		const NCryDX12::CView& View = it->GetDX12View();
		NCryDX12::CResource& Resource = View.GetDX12Resource();
		const D3D12_RESOURCE_STATES desiredState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		if (Resource.InitHasBeenDeferred())
		{
			Resource.InitDeferred(pCommandListDX12, desiredState);
		}

		assert(!((pCommandListDX12->GetD3D12ListType() != D3D12_COMMAND_LIST_TYPE_DIRECT) && (Resource.GetTargetState() & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) && (!Resource.IsOffCard())));

		pCommandListDX12->PrepareResourceUAVUsage(Resource, View, desiredState);
	}

	pResourcesDX12->MarkInUse();
}

void CDeviceGraphicsCommandInterfaceImpl::PrepareInlineConstantBufferForUseImpl(uint32 bindSlot, CConstantBuffer* pConstantBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderClass) const
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;

	NCryDX12::CResource& Resource = GET_DX12_CONSTANTBUFFER_RESOURCE(pConstantBuffer)->GetDX12Resource();
	const D3D12_RESOURCE_STATES desiredState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

	pCommandListDX12->PrepareResourceCBVUsage(Resource, desiredState);
}

void CDeviceGraphicsCommandInterfaceImpl::PrepareInlineConstantBufferForUseImpl(uint32 bindSlot, CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, ::EShaderStage shaderStages) const
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	PrepareInlineConstantBufferForUseImpl(bindSlot, pBuffer, shaderSlot, eHWSC_Num);
}

void CDeviceGraphicsCommandInterfaceImpl::PrepareVertexBuffersForUseImpl(uint32 numStreams, uint32 lastStreamSlot, const CDeviceInputStream* vertexStreams) const
{
	if (m_graphicsState.vertexStreams != vertexStreams)
	{
		NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;

		for (uint32 s = 0; s < numStreams; ++s)
		{
			const CDeviceInputStream& vertexStream = vertexStreams[s];

			CRY_ASSERT(vertexStream.hStream != ~0u);
			{
				// TODO: try not to call GetD3D() here, overhead: 1 call (and no inlinening) + 1 look-up + 2 accesses + branch
				size_t offset;
				auto pBuffer = reinterpret_cast<CCryDX12Buffer*>(gcpRendD3D.m_DevBufMan.GetD3D(vertexStream.hStream, &offset));
				NCryDX12::CResource& Resource = pBuffer->GetDX12Resource();
				const D3D12_RESOURCE_STATES desiredState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

				pCommandListDX12->PrepareResourceVBVUsage(Resource, desiredState);
			}
		}
	}
}

void CDeviceGraphicsCommandInterfaceImpl::PrepareIndexBufferForUseImpl(const CDeviceInputStream* indexStream) const
{
	if (m_graphicsState.indexStream != indexStream)
	{
		NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;

		CRY_ASSERT(indexStream->hStream != ~0u);
		{
			// TODO: try not to call GetD3D() here, overhead: 1 call (and no inlinening) + 1 look-up + 2 accesses + branch
			size_t offset;
			auto pBuffer = reinterpret_cast<CCryDX12Buffer*>(gcpRendD3D.m_DevBufMan.GetD3D(indexStream->hStream, &offset));
			NCryDX12::CResource& Resource = pBuffer->GetDX12Resource();
			const D3D12_RESOURCE_STATES desiredState = D3D12_RESOURCE_STATE_INDEX_BUFFER;

			pCommandListDX12->PrepareResourceIBVUsage(Resource, desiredState);
		}
	}
}

void CDeviceGraphicsCommandInterfaceImpl::BeginResourceTransitionsImpl(uint32 numTextures, CTexture** pTextures, EResourceTransitionType type)
{
	if (!CRenderer::CV_r_D3D12EarlyResourceBarriers)
		return;

	NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;

	uint32 numBarriers = 0;
	for (uint32 i = 0; i < numTextures; i++)
	{
		CCryDX12Resource<ID3D11ResourceToImplement>* pResource = nullptr;
		if (pTextures[i] && pTextures[i]->GetDevTexture())
		{
			pResource = DX12_EXTRACT_RESOURCE(pTextures[i]->GetDevTexture()->GetBaseTexture());
		}

		if (pResource != nullptr)
		{
			NCryDX12::CResource& resource = pResource->GetDX12Resource();
			const D3D12_RESOURCE_STATES desiredState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

			if (resource.NeedsTransitionBarrier(pCommandListDX12, desiredState))
			{
				pCommandListDX12->MaxResourceFenceValue(resource, CMDTYPE_WRITE);
				pCommandListDX12->BeginResourceStateTransition(resource, desiredState);
				pCommandListDX12->SetResourceFenceValue(resource, CMDTYPE_READ);
				numBarriers += 1;
			}
		}
	}

	if (numBarriers > 0)
	{
		pCommandListDX12->PendingResourceBarriers();

		if (CRenderer::CV_r_D3D12EarlyResourceBarriers > 1)
			gcpRendD3D->GetDeviceContext_Unsynchronized().Flush();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeviceGraphicsCommandInterfaceImpl::SetRenderTargetsImpl(uint32 targetCount, CTexture* const* pTargets, const SDepthTexture* pDepthTarget, const SResourceView::KeyType* pRenderTargetViews /*= nullptr*/)
{
	NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;

	const NCryDX12::CView* pDSV = nullptr;
	const NCryDX12::CView* pRTV[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = { nullptr };

	// Get current depth stencil views
	if (pDepthTarget)
	{
		const NCryDX12::CView& View = GET_DX12_DEPTHSTENCIL_VIEW(pDepthTarget)->GetDX12View();
		NCryDX12::CResource& Resource = View.GetDX12Resource();

		pDSV = &View;

		const D3D12_RESOURCE_STATES desiredState = Resource.GetDesc().Flags & D3D11_DSV_READ_ONLY_DEPTH ? D3D12_RESOURCE_STATE_DEPTH_READ : D3D12_RESOURCE_STATE_DEPTH_WRITE;
		assert(!Resource.NeedsTransitionBarrier(pCommandListDX12, View, desiredState));
	}

	// Get current render target views
	for (int i = 0; i < targetCount; ++i)
	{
		if (pTargets[i])
		{
			const bool bHasCustomView = pRenderTargetViews && pRenderTargetViews[i] != SResourceView::DefaultRendertargetView;
			const SResourceView::KeyType rtvKey = bHasCustomView ? pRenderTargetViews[i] : SResourceView::RenderTargetView(pTargets[i]->GetDstFormat()).m_Desc.Key;
			CRY_ASSERT(SResourceView::IsRenderTargetView(rtvKey));

			const NCryDX12::CView& UnpatchedView = GET_DX12_RENDERTARGET_VIEW(pTargets[i], rtvKey)->GetDX12View();
			pRTV[i] = &pCommandListDX12->PatchRenderTargetView(UnpatchedView);
			NCryDX12::CResource& Resource = pRTV[i]->GetDX12Resource();

			assert(!Resource.NeedsTransitionBarrier(pCommandListDX12, *pRTV[i], D3D12_RESOURCE_STATE_RENDER_TARGET));
		}
	}

	// TODO: if we know early that the resource(s) will be RENDER_TARGET/DEPTH_READ|WRITE we can begin the barrier early and end it here
	pCommandListDX12->BindAndSetOutputViews(targetCount, pRTV, pDSV);
}

void CDeviceGraphicsCommandInterfaceImpl::SetViewportsImpl(uint32 vpCount, const D3DViewPort* pViewports)
{
	// D3D11_VIEWPORT := D3D12_VIEWPORT
	m_sharedState.pCommandList->SetViewports(vpCount, (D3D12_VIEWPORT*)pViewports);
}

void CDeviceGraphicsCommandInterfaceImpl::SetScissorRectsImpl(uint32 rcCount, const D3DRectangle* pRects)
{
	// D3D11_RECT := D3D12_RECT
	m_sharedState.pCommandList->SetScissorRects(rcCount, (D3D12_RECT*)pRects);
}

void CDeviceGraphicsCommandInterfaceImpl::SetPipelineStateImpl(const CDeviceGraphicsPSO* devicePSO)
{
	auto pCommandListDX12 = m_sharedState.pCommandList;
	const CDeviceGraphicsPSO_DX12* pDevicePSO = reinterpret_cast<const CDeviceGraphicsPSO_DX12*>(devicePSO);

	pCommandListDX12->SetPSO(pDevicePSO->GetGraphicsPSO());
	m_computeState.pPipelineState = nullptr; // on dx12 pipeline state is shared between graphics and compute

	D3D12_PRIMITIVE_TOPOLOGY psoPrimitiveTopology = pDevicePSO->GetPrimitiveTopology();
	if (m_graphicsState.custom.primitiveTopology.Set(psoPrimitiveTopology))
	{
	#if defined(ENABLE_PROFILING_CODE)
		CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumTopologySets);
	#endif

		pCommandListDX12->SetPrimitiveTopology(psoPrimitiveTopology);
	}
}

void CDeviceGraphicsCommandInterfaceImpl::SetResourceLayoutImpl(const CDeviceResourceLayout* resourceLayout)
{
	const CDeviceResourceLayout_DX12* pResourceLayoutDX12 = reinterpret_cast<const CDeviceResourceLayout_DX12*>(resourceLayout);
	m_sharedState.pCommandList->SetGraphicsRootSignature(pResourceLayoutDX12->GetRootSignature());
}

void CDeviceGraphicsCommandInterfaceImpl::SetVertexBuffersImpl(uint32 numStreams, uint32 lastStreamSlot, const CDeviceInputStream* vertexStreams)
{
	NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;
	pCommandListDX12->ClearVertexBufferHeap(lastStreamSlot + 1);

	for (uint32 s = 0; s < numStreams; ++s)
	{
		const CDeviceInputStream& vertexStream = vertexStreams[s];

		CRY_ASSERT(vertexStream.hStream != ~0u);
		{
			// TODO: try not to call GetD3D() here, overhead: 1 call (and no inlinening) + 1 look-up + 2 accesses + branch
			size_t offset;
			auto* pBuffer = reinterpret_cast<CCryDX12Buffer*>(gcpRendD3D.m_DevBufMan.GetD3D(vertexStream.hStream, &offset));

			assert(!pBuffer->GetDX12Resource().NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	#if defined(ENABLE_PROFILING_CODE)
			CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundVertexBuffers[pBuffer->GetDX12Resource().IsOffCard()]);
	#endif

			pCommandListDX12->BindVertexBufferView(pBuffer->GetDX12View(), vertexStream.nSlot, TRange<uint32>(offset, offset), vertexStream.nStride);
		}
	}

	// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
	pCommandListDX12->SetVertexBufferHeap(lastStreamSlot + 1);
}

void CDeviceGraphicsCommandInterfaceImpl::SetIndexBufferImpl(const CDeviceInputStream* indexStream)
{
	NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;

	CRY_ASSERT(indexStream->hStream != ~0u);
	{
		// TODO: try not to call GetD3D() here, overhead: 1 call (and no inlinening) + 1 look-up + 2 accesses + branch
		size_t offset;
		auto* pBuffer = reinterpret_cast<CCryDX12Buffer*>(gcpRendD3D.m_DevBufMan.GetD3D(indexStream->hStream, &offset));

		assert(!pBuffer->GetDX12Resource().NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_INDEX_BUFFER));

	#if defined(ENABLE_PROFILING_CODE)
		CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundIndexBuffers[pBuffer->GetDX12Resource().IsOffCard()]);
	#endif

		// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
	#if !defined(SUPPORT_FLEXIBLE_INDEXBUFFER)
		pCommandListDX12->BindAndSetIndexBufferView(pBuffer->GetDX12View(), DXGI_FORMAT_R16_UINT, offset);
	#else
		pCommandListDX12->BindAndSetIndexBufferView(pBuffer->GetDX12View(), (DXGI_FORMAT)indexStream->nStride, UINT(offset));
	#endif
	}
}

void CDeviceGraphicsCommandInterfaceImpl::SetResourcesImpl(uint32 bindSlot, const CDeviceResourceSet* pResources, ::EShaderStage srvUsage)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);

	NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;
	const CDeviceResourceSet_DX12* pResourcesDX12 = reinterpret_cast<const CDeviceResourceSet_DX12*>(pResources);

	for (auto& it : pResourcesDX12->m_ConstantBuffersInUse)
	{
		NCryDX12::CResource& Resource = it->GetDX12Resource();

		assert(!Resource.NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	#if defined(ENABLE_PROFILING_CODE)
		CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundConstBuffers[Resource.IsOffCard()]);
	#endif

		// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
		pCommandListDX12->TrackResourceCBVUsage(Resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	}

	for (auto& it : pResourcesDX12->m_ShaderResourceViewsInUse)
	{
		const NCryDX12::CView& View = it->GetDX12View();
		NCryDX12::CResource& Resource = View.GetDX12Resource();
		const D3D12_RESOURCE_STATES desiredState =
			(srvUsage & EShaderStage_Pixel ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_COMMON) |
			(srvUsage & EShaderStage_Compute ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_COMMON);

		assert(!Resource.NeedsTransitionBarrier(pCommandListDX12, View, desiredState));

	#if defined(ENABLE_PROFILING_CODE)
		const bool bIsBuffer = Resource.GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER;
		if (bIsBuffer) CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundUniformBuffers[Resource.IsOffCard()]);
		else           CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundUniformTextures[Resource.IsOffCard()]);
	#endif

		// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
		pCommandListDX12->TrackResourcePRVUsage(Resource, View, desiredState);
	}

	for (auto& it : pResourcesDX12->m_UnorderedAccessViewsInUse)
	{
		const NCryDX12::CView& View = it->GetDX12View();
		NCryDX12::CResource& Resource = View.GetDX12Resource();

		assert(!Resource.NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	#if defined(ENABLE_PROFILING_CODE)
		const bool bIsBuffer = Resource.GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER;
		if (bIsBuffer) CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundUniformBuffers[Resource.IsOffCard()]);
		else           CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundUniformTextures[Resource.IsOffCard()]);
	#endif

		// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
		pCommandListDX12->TrackResourceUAVUsage(Resource, View, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	const CDescriptorBlock& descriptorBlock = pResourcesDX12->GetDescriptorBlock();
	pCommandListDX12->SetGraphicsDescriptorTable(bindSlot, descriptorBlock.GetHandleOffsetGPU(0));
}

void CDeviceGraphicsCommandInterfaceImpl::SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, ::EShaderStage shaderStages)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	SetInlineConstantBufferImpl(bindSlot, pBuffer, shaderSlot, eHWSC_Num);
}

void CDeviceGraphicsCommandInterfaceImpl::SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pConstantBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderClass)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;

	const NCryDX12::CView& View = GET_DX12_CONSTANTBUFFER_RESOURCE(pConstantBuffer)->GetDX12View();
	NCryDX12::CResource& Resource = View.GetDX12Resource();

	assert(!Resource.NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	#if defined(ENABLE_PROFILING_CODE)
	CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundInlineBuffers[Resource.IsOffCard()]);
	#endif

	// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
	pCommandListDX12->TrackResourceCBVUsage(Resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = View.GetCBVDesc().BufferLocation + pConstantBuffer->m_offset;
	pCommandListDX12->SetGraphicsConstantBufferView(bindSlot, gpuAddress);
}

void CDeviceGraphicsCommandInterfaceImpl::SetInlineConstantsImpl(uint32 bindSlot, uint32 constantCount, float* pConstants)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	m_sharedState.pCommandList->SetGraphics32BitConstants(bindSlot, constantCount, pConstants, 0);
}

void CDeviceGraphicsCommandInterfaceImpl::SetStencilRefImpl(uint8 stencilRefValue)
{
	m_sharedState.pCommandList->SetStencilRef(stencilRefValue);
}

void CDeviceGraphicsCommandInterfaceImpl::SetDepthBiasImpl(float constBias, float slopeBias, float biasClamp)
{
	CRY_ASSERT_MESSAGE(false, "Depth bias can only be set via PSO on DirectX 12");
}

void CDeviceGraphicsCommandInterfaceImpl::DrawImpl(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
	m_sharedState.pCommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void CDeviceGraphicsCommandInterfaceImpl::DrawIndexedImpl(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, int BaseVertexLocation, uint32 StartInstanceLocation)
{
	m_sharedState.pCommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void CDeviceGraphicsCommandInterfaceImpl::ClearSurfaceImpl(D3DSurface* pView, const FLOAT Color[4], UINT NumRects, const D3D11_RECT* pRects)
{
	const NCryDX12::CView& View = reinterpret_cast<CCryDX12RenderTargetView*>(pView)->GetDX12View();
	m_sharedState.pCommandList->ClearRenderTargetView(View, Color, NumRects, pRects);
}

void CDeviceGraphicsCommandInterfaceImpl::ClearSurfaceImpl(D3DDepthSurface* pView, int clearFlags, float depth, uint8 stencil, uint32 numRects, const D3D11_RECT* pRects)
{
	const NCryDX12::CView& View = reinterpret_cast<CCryDX12DepthStencilView*>(pView)->GetDX12View();
	m_sharedState.pCommandList->ClearDepthStencilView(View, D3D12_CLEAR_FLAGS(clearFlags), depth, stencil, numRects, pRects);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeviceComputeCommandInterfaceImpl::PrepareUAVsForUseImpl(uint32 viewCount, CGpuBuffer** pViews) const
{
	NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;

	for (int v = 0; v < viewCount; ++v)
	{
		const NCryDX12::CView& View = reinterpret_cast<CCryDX12UnorderedAccessView*>(pViews[v]->GetDeviceUAV())->GetDX12View();
		NCryDX12::CResource& Resource = View.GetDX12Resource();
		const D3D12_RESOURCE_STATES desiredState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		if (Resource.InitHasBeenDeferred())
		{
			Resource.InitDeferred(pCommandListDX12, desiredState);
		}

		assert(!((pCommandListDX12->GetD3D12ListType() != D3D12_COMMAND_LIST_TYPE_DIRECT) && (Resource.GetTargetState() & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) && (!Resource.IsOffCard())));

		pCommandListDX12->PrepareResourceUAVUsage(Resource, View, desiredState);
	}
}

void CDeviceComputeCommandInterfaceImpl::PrepareResourcesForUseImpl(uint32 bindSlot, CDeviceResourceSet* pResources, ::EShaderStage srvUsage) const
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);

	NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;
	CDeviceResourceSet_DX12* pResourcesDX12 = reinterpret_cast<CDeviceResourceSet_DX12*>(pResources);

	if (pResourcesDX12->m_Flags & CDeviceResourceSet::EFlags_Multibuffered)
	{
		pResourcesDX12->PatchDescriptors(GetDevice()->GetResourceDescriptorScratchSpace());
	}

	for (auto& it : pResourcesDX12->m_ConstantBuffersInUse)
	{
		NCryDX12::CResource& Resource = it->GetDX12Resource();
		const D3D12_RESOURCE_STATES desiredState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

		if (Resource.InitHasBeenDeferred())
		{
			Resource.InitDeferred(pCommandListDX12, desiredState);
		}

		pCommandListDX12->PrepareResourceCBVUsage(Resource, desiredState);
	}

	for (auto& it : pResourcesDX12->m_ShaderResourceViewsInUse)
	{
		const NCryDX12::CView& View = it->GetDX12View();
		NCryDX12::CResource& Resource = View.GetDX12Resource();
		const D3D12_RESOURCE_STATES desiredState =
			(srvUsage & EShaderStage_Pixel ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_COMMON) |
			(srvUsage & EShaderStage_Compute ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_COMMON);

		if (Resource.InitHasBeenDeferred())
		{
			Resource.InitDeferred(pCommandListDX12, desiredState);
		}

		assert(!((pCommandListDX12->GetD3D12ListType() != D3D12_COMMAND_LIST_TYPE_DIRECT) && (Resource.GetTargetState() & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) && (!Resource.IsOffCard())));

		pCommandListDX12->PrepareResourceCRVUsage(Resource, View, desiredState);
	}

	for (auto& it : pResourcesDX12->m_UnorderedAccessViewsInUse)
	{
		const NCryDX12::CView& View = it->GetDX12View();
		NCryDX12::CResource& Resource = View.GetDX12Resource();
		const D3D12_RESOURCE_STATES desiredState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		if (Resource.InitHasBeenDeferred())
		{
			Resource.InitDeferred(pCommandListDX12, desiredState);
		}

		assert(!((pCommandListDX12->GetD3D12ListType() != D3D12_COMMAND_LIST_TYPE_DIRECT) && (Resource.GetTargetState() & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) && (!Resource.IsOffCard())));

		pCommandListDX12->PrepareResourceUAVUsage(Resource, View, desiredState);
	}

	pResourcesDX12->MarkInUse();
}

void CDeviceComputeCommandInterfaceImpl::PrepareInlineConstantBufferForUseImpl(uint32 bindSlot, CConstantBuffer* pConstantBuffer, EConstantBufferShaderSlot shaderSlot) const
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;

	NCryDX12::CResource& Resource = GET_DX12_CONSTANTBUFFER_RESOURCE(pConstantBuffer)->GetDX12Resource();
	const D3D12_RESOURCE_STATES desiredState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

	pCommandListDX12->PrepareResourceCBVUsage(Resource, desiredState);
}

void CDeviceComputeCommandInterfaceImpl::SetPipelineStateImpl(const CDeviceComputePSO* pDevicePSO)
{
	const CDeviceComputePSO_DX12* pDevicePsoDX12 = reinterpret_cast<const CDeviceComputePSO_DX12*>(pDevicePSO);
	m_sharedState.pCommandList->SetPSO(pDevicePsoDX12->GetComputePSO());
	m_graphicsState.pPipelineState = nullptr; // on dx12 pipeline state is shared between graphics and compute
}

void CDeviceComputeCommandInterfaceImpl::SetResourceLayoutImpl(const CDeviceResourceLayout* pResourceLayout)
{
	const CDeviceResourceLayout_DX12* pResourceLayoutDX12 = reinterpret_cast<const CDeviceResourceLayout_DX12*>(pResourceLayout);
	m_sharedState.pCommandList->SetComputeRootSignature(pResourceLayoutDX12->GetRootSignature());
}

void CDeviceComputeCommandInterfaceImpl::SetResourcesImpl(uint32 bindSlot, const CDeviceResourceSet* pResources, ::EShaderStage srvUsage)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);

	NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;
	const CDeviceResourceSet_DX12* pResourcesDX12 = reinterpret_cast<const CDeviceResourceSet_DX12*>(pResources);

	for (auto& it : pResourcesDX12->m_ConstantBuffersInUse)
	{
		NCryDX12::CResource& Resource = it->GetDX12Resource();

		assert(!Resource.NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	#if defined(ENABLE_PROFILING_CODE)
		CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundConstBuffers[Resource.IsOffCard()]);
	#endif

		// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
		pCommandListDX12->TrackResourceCBVUsage(Resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	}

	for (auto& it : pResourcesDX12->m_ShaderResourceViewsInUse)
	{
		const NCryDX12::CView& View = it->GetDX12View();
		NCryDX12::CResource& Resource = View.GetDX12Resource();
		const D3D12_RESOURCE_STATES desiredState =
			(srvUsage & EShaderStage_Pixel ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_COMMON) |
			(srvUsage & EShaderStage_Compute ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_COMMON);

		assert(!Resource.NeedsTransitionBarrier(pCommandListDX12, View, desiredState));

	#if defined(ENABLE_PROFILING_CODE)
		CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundUniformTextures[Resource.IsOffCard()]);
	#endif

		// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
		pCommandListDX12->TrackResourceCRVUsage(Resource, View, desiredState);
	}

	for (auto& it : pResourcesDX12->m_UnorderedAccessViewsInUse)
	{
		const NCryDX12::CView& View = it->GetDX12View();
		NCryDX12::CResource& Resource = View.GetDX12Resource();

		assert(!Resource.NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	#if defined(ENABLE_PROFILING_CODE)
		CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundUniformBuffers[Resource.IsOffCard()]);
	#endif

		// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
		pCommandListDX12->TrackResourceUAVUsage(Resource, View, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	const CDescriptorBlock& descriptorBlock = pResourcesDX12->GetDescriptorBlock();
	pCommandListDX12->SetComputeDescriptorTable(bindSlot, descriptorBlock.GetHandleOffsetGPU(0));
}

void CDeviceComputeCommandInterfaceImpl::SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pConstantBuffer, EConstantBufferShaderSlot shaderSlot)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	NCryDX12::CCommandList* pCommandListDX12 = m_sharedState.pCommandList;

	const NCryDX12::CView& View = GET_DX12_CONSTANTBUFFER_RESOURCE(pConstantBuffer)->GetDX12View();
	NCryDX12::CResource& Resource = View.GetDX12Resource();

	assert(!Resource.NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	#if defined(ENABLE_PROFILING_CODE)
	CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundInlineBuffers[Resource.IsOffCard()]);
	#endif

	// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
	pCommandListDX12->TrackResourceCBVUsage(Resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = View.GetCBVDesc().BufferLocation + pConstantBuffer->m_offset;
	pCommandListDX12->SetComputeConstantBufferView(bindSlot, gpuAddress);
}

void CDeviceComputeCommandInterfaceImpl::SetInlineConstantsImpl(uint32 bindSlot, uint32 constantCount, float* pConstants)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	m_sharedState.pCommandList->SetCompute32BitConstants(bindSlot, constantCount, pConstants, 0);
}

void CDeviceComputeCommandInterfaceImpl::DispatchImpl(uint32 X, uint32 Y, uint32 Z)
{
	m_sharedState.pCommandList->Dispatch(X, Y, Z);
}

void CDeviceComputeCommandInterfaceImpl::ClearUAVImpl(D3DUAV* pView, const FLOAT Values[4], UINT NumRects, const D3D11_RECT* pRects)
{
	const NCryDX12::CView& View = reinterpret_cast<CCryDX12UnorderedAccessView*>(pView)->GetDX12View();
	m_sharedState.pCommandList->ClearUnorderedAccessView(View, Values, NumRects, pRects);
}

void CDeviceComputeCommandInterfaceImpl::ClearUAVImpl(D3DUAV* pView, const UINT Values[4], UINT NumRects, const D3D11_RECT* pRects)
{
	const NCryDX12::CView& View = reinterpret_cast<CCryDX12UnorderedAccessView*>(pView)->GetDX12View();
	m_sharedState.pCommandList->ClearUnorderedAccessView(View, Values, NumRects, pRects);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDeviceObjectFactory::CDeviceObjectFactory()
{
	m_pCoreCommandList = std::make_shared<CDeviceCommandList>();
	m_pCoreCommandList->m_sharedState.pCommandList = nullptr;
}

CDeviceGraphicsPSOPtr CDeviceObjectFactory::CreateGraphicsPSOImpl(const CDeviceGraphicsPSODesc& psoDesc) const
{
	auto pResult = std::make_shared<CDeviceGraphicsPSO_DX12>(GetDevice());
	pResult->Init(psoDesc);

	return pResult;
}

CDeviceComputePSOPtr CDeviceObjectFactory::CreateComputePSOImpl(const CDeviceComputePSODesc& psoDesc) const
{
	auto pResult = std::make_shared<CDeviceComputePSO_DX12>(GetDevice());
	pResult->Init(psoDesc);

	return pResult;
}

CDeviceResourceSetPtr CDeviceObjectFactory::CreateResourceSet(CDeviceResourceSet::EFlags flags) const
{
	return std::make_shared<CDeviceResourceSet_DX12>(flags);
}
CDeviceResourceSetPtr CDeviceObjectFactory::CloneResourceSet(const CDeviceResourceSetPtr pSrcResourceSet) const
{
	return std::make_shared<CDeviceResourceSet_DX12>(*pSrcResourceSet);
}

CDeviceResourceLayoutPtr CDeviceObjectFactory::CreateResourceLayoutImpl(const SDeviceResourceLayoutDesc& resourceLayoutDesc) const
{
	auto pResult = std::make_shared<CDeviceResourceLayout_DX12>(GetDevice(), resourceLayoutDesc.GetRequiredResourceBindings());
	if (pResult->Init(resourceLayoutDesc))
		return pResult;

	return nullptr;
}

CDeviceCommandListPtr CDeviceObjectFactory::GetCoreCommandList() const
{
	auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(gcpRendD3D->GetDeviceContext().GetRealDeviceContext());
	m_pCoreCommandList->m_sharedState.pCommandList = pContext->GetCoreGraphicsCommandList();

	return m_pCoreCommandList;
}

// Helper functions for DX12 MultiGPU
ID3D12CommandQueue* CDeviceObjectFactory::GetNativeCoreCommandQueue()
{
	auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(gcpRendD3D->GetDeviceContext().GetRealDeviceContext());
	ID3D12CommandQueue* pD3d12Queue = pContext->GetCoreCommandListPool(CMDQUEUE_GRAPHICS).GetD3D12CommandQueue();

#ifdef CRY_USE_DX12_MULTIADAPTER
	if (pContext->GetDevice()->GetDX12Device()->IsMultiAdapter())
	{
		BroadcastableD3D12CommandQueue<2>* pBroadcastQueue = reinterpret_cast<BroadcastableD3D12CommandQueue<2>*>(pD3d12Queue);
		pD3d12Queue = *(*pBroadcastQueue)[0];
	}
#endif

	return pD3d12Queue;
}

// Acquire one or more command-lists which are independent of the core command-list
// Only one thread is allowed to call functions on this command-list (DX12 restriction).
// The thread that gets the permition is the one calling Begin() on it AFAIS
CDeviceCommandListUPtr CDeviceObjectFactory::AcquireCommandList(EQueueType eQueueType /*= eQueue_Graphics*/)
{
	// In theory this whole function need to be atomic, instead it voids the core command-list(s),
	// Synchronization between different threads acquiring command-lists is deferred to the higher level.
	auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(gcpRendD3D->GetDeviceContext().GetRealDeviceContext());
	NCryDX12::CCommandListPool& pQueue = pContext->GetCoreCommandListPool(eQueueType);
	pContext->CeaseAllCommandQueues(false);

	DX12_PTR(CCommandList) pCL;
	pQueue.AcquireCommandList(pCL);

	pContext->ResumeAllCommandQueues();
	GetCoreCommandList()->Reset();
	auto pResult = CryMakeUnique<CDeviceCommandList>();
	pResult->m_sharedState.pCommandList = pCL;
	return pResult;
}

std::vector<CDeviceCommandListUPtr> CDeviceObjectFactory::AcquireCommandLists(uint32 listCount, EQueueType eQueueType /*= eQueue_Graphics*/)
{
	// In theory this whole function need to be atomic, instead it voids the core command-list(s),
	// Synchronization between different threads acquiring command-lists is deferred to the higher level.
	auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(gcpRendD3D->GetDeviceContext().GetRealDeviceContext());
	NCryDX12::CCommandListPool& pQueue = pContext->GetCoreCommandListPool(eQueueType);
	pContext->CeaseAllCommandQueues(false);

	std::vector<CDeviceCommandListUPtr> pCommandLists;
	DX12_PTR(CCommandList) pCLs[256];

	// Allocate in chunks of 256
	for (uint32 n = 0; n < listCount; n += 256U)
	{
		const uint32 chunkCount = std::min(listCount - n, 256U);
		pQueue.AcquireCommandLists(chunkCount, pCLs);

		for (uint32 b = 0; b < chunkCount; ++b)
		{
			pCommandLists.emplace_back(CryMakeUnique<CDeviceCommandList>());
			pCommandLists.back()->m_sharedState.pCommandList = pCLs[b];
		}
	}

	pContext->ResumeAllCommandQueues();
	GetCoreCommandList()->Reset();
	return std::move(pCommandLists);
}

// Command-list sinks, will automatically submit command-lists in allocation-order
void CDeviceObjectFactory::ForfeitCommandList(CDeviceCommandListUPtr pCommandList, EQueueType eQueueType /*= eQueue_Graphics*/)
{
	auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(gcpRendD3D->GetDeviceContext().GetRealDeviceContext());
	NCryDX12::CCommandListPool& pQueue = pContext->GetCoreCommandListPool(eQueueType);

	if (pCommandList)
	{
		pQueue.ForfeitCommandList(pCommandList->m_sharedState.pCommandList);
	}
}

void CDeviceObjectFactory::ForfeitCommandLists(std::vector<CDeviceCommandListUPtr> pCommandLists, EQueueType eQueueType /*= eQueue_Graphics*/)
{
	auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(gcpRendD3D->GetDeviceContext().GetRealDeviceContext());
	NCryDX12::CCommandListPool& pQueue = pContext->GetCoreCommandListPool(eQueueType);

	const uint32 listCount = pCommandLists.size();
	DX12_PTR(CCommandList) pCLs[256];

	// Deallocate in chunks of 256
	for (uint32 n = 0; n < listCount; n += 256U)
	{
		const uint32 chunkCount = std::min(listCount - n, 256U);
		uint32 validCount = 0;

		for (uint32 b = 0; b < chunkCount; ++b)
		{
			if (pCommandLists[b])
			{
				pCLs[validCount++] = pCommandLists[b]->m_sharedState.pCommandList;
			}
		}

		if (validCount)
		{
			pQueue.ForfeitCommandLists(validCount, pCLs);
		}
	}
}

void CDeviceObjectFactory::ReleaseResourcesImpl()
{
	auto pDevice = reinterpret_cast<CCryDX12Device*>(gcpRendD3D->GetDevice().GetRealDevice());
	pDevice->FlushAndWaitForGPU();
}

#endif
