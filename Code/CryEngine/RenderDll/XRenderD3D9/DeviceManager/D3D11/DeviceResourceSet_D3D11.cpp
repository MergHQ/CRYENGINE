// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "../DeviceObjects.h"
#include "../../../Common/ReverseDepth.h"

#include "DeviceResourceSet_D3D11.h"

const void* const CDeviceResourceSet_DX11::InvalidPointer = (const void* const)0xBADA55;

void CDeviceResourceSet_DX11::ClearCompiledData()
{
	// Releasing resources is allowed to be done by any thread, just not concurrently
	ZeroMemory(&compiledCBs, sizeof(compiledCBs));
	ZeroMemory(&numCompiledCBs, sizeof(numCompiledCBs));

	compiledSRVs.Clear();
	compiledUAVs.Clear();
	compiledSamplers.Clear();
}

bool CDeviceResourceSet_DX11::UpdateImpl(const CDeviceResourceSetDesc& desc, CDeviceResourceSetDesc::EDirtyFlags dirtyFlags)
{
	CRY_ASSERT(gRenDev->m_pRT->IsRenderThread());

	ClearCompiledData();

	for (const auto& it : desc.GetResources())
	{
		const SResourceBindPoint& bindPoint = it.first;
		const SResourceBinding&   resource = it.second;

		CRY_ASSERT(bindPoint.slotNumber < MAX_TMU);

		if (!resource.IsValid())
		{
			CRY_ASSERT(false, "Invalid resource in resource set desc. Update failed");
			return false;
		}

		switch (bindPoint.slotType)
		{
		case SResourceBindPoint::ESlotType::ConstantBuffer:
		{
			SCompiledConstantBuffer compiledCB;

			buffer_size_t offset, size;

			compiledCB.pBuffer = resource.pConstantBuffer->GetD3D(&offset, &size);
			compiledCB.code = resource.pConstantBuffer->GetCode();
			compiledCB.offset = offset;
			compiledCB.size = size;
			compiledCB.slot = EConstantBufferShaderSlot(it.first.slotNumber);

			// Shader stages are ordered by usage-frequency and loop exists according to usage-frequency (VS+PS fast, etc.)
			int validShaderStages = it.first.stages;
			for (EHWShaderClass shaderClass = eHWSC_Vertex; validShaderStages; shaderClass = EHWShaderClass(shaderClass + 1), validShaderStages >>= 1)
			{
				if (validShaderStages & 1)
					compiledCBs[shaderClass][numCompiledCBs[shaderClass]++] = compiledCB;
			}
		}
		break;

		case SResourceBindPoint::ESlotType::TextureAndBuffer:
		{
			CDeviceResource* pResource = uint8(bindPoint.flags & SResourceBindPoint::EFlags::IsTexture)
				? (CDeviceResource*)resource.pTexture->GetDevTexture()
				: (CDeviceResource*)resource.pBuffer->GetDevBuffer();

			ID3D11ShaderResourceView* pSRV = pResource->LookupSRV(resource.view);
			compiledSRVs.AddResource(pSRV, bindPoint);
		};
		break;

		case SResourceBindPoint::ESlotType::UnorderedAccessView:
		{
			CRY_ASSERT((bindPoint.stages & (EShaderStage_Vertex | EShaderStage_Domain | EShaderStage_Hull | EShaderStage_Geometry)) == 0);

			CDeviceResource* pResource = uint8(bindPoint.flags & SResourceBindPoint::EFlags::IsTexture)
				? (CDeviceResource*)resource.pTexture->GetDevTexture()
				: (CDeviceResource*)resource.pBuffer->GetDevBuffer();

			ID3D11UnorderedAccessView* pUAV = pResource->LookupUAV(resource.view);
			compiledUAVs.AddResource(pUAV, bindPoint);
		};
		break;

		case SResourceBindPoint::ESlotType::Sampler:
		{
			ID3D11SamplerState* pSamplerState = CDeviceObjectFactory::LookupSamplerState(resource.samplerState).second;
			compiledSamplers.AddResource(pSamplerState, bindPoint);
		}
		break;
		}

	}

	return true;
}

