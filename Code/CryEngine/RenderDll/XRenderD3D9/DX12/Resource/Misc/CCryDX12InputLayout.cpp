// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CCryDX12InputLayout.hpp"

CCryDX12InputLayout* CCryDX12InputLayout::Create(CCryDX12Device* pDevice, const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs11, UINT NumElements, const void* pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength)
{
	ID3D12Device* pDevice12 = pDevice->GetD3D12Device();
	CCryDX12InputLayout* pResult = DX12_NEW_RAW(CCryDX12InputLayout());
	pResult->m_Descriptors.resize(NumElements);

	for (UINT i = 0; i < NumElements; ++i)
	{
		pResult->m_SemanticNames.push_back(pInputElementDescs11[i].SemanticName);
	}

	for (UINT i = 0; i < NumElements; ++i)
	{
		D3D12_INPUT_ELEMENT_DESC& desc12 = pResult->m_Descriptors[i];
		ZeroMemory(&desc12, sizeof(D3D12_INPUT_ELEMENT_DESC));

		desc12.SemanticName = pResult->m_SemanticNames[i].c_str();
		desc12.SemanticIndex = pInputElementDescs11[i].SemanticIndex;
		desc12.Format = pInputElementDescs11[i].Format;
		desc12.InputSlot = pInputElementDescs11[i].InputSlot;
		desc12.AlignedByteOffset = pInputElementDescs11[i].AlignedByteOffset;
		desc12.InputSlotClass = static_cast<D3D12_INPUT_CLASSIFICATION>(pInputElementDescs11[i].InputSlotClass);
		desc12.InstanceDataStepRate = pInputElementDescs11[i].InstanceDataStepRate;
	}

	return pResult;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12InputLayout::CCryDX12InputLayout()
	: Super(nullptr, nullptr)
{

}

CCryDX12InputLayout::~CCryDX12InputLayout()
{

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

size_t CCryDX12InputLayout::GetInputSize(UINT inputSlot) const
{
	size_t size = 0;
	for (UINT i = 0; i < m_Descriptors.size(); ++i)
	{
		if (m_Descriptors[i].InputSlot == inputSlot)
			switch (m_Descriptors[i].Format)
			{
			default:
			case DXGI_FORMAT_UNKNOWN:
				size += 0;
				break;

			case DXGI_FORMAT_R32G32B32A32_TYPELESS:
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
			case DXGI_FORMAT_R32G32B32A32_UINT:
			case DXGI_FORMAT_R32G32B32A32_SINT:
				size += 4 * sizeof(float);
				break;

			case DXGI_FORMAT_R32G32B32_TYPELESS:
			case DXGI_FORMAT_R32G32B32_FLOAT:
			case DXGI_FORMAT_R32G32B32_UINT:
			case DXGI_FORMAT_R32G32B32_SINT:
				size += 3 * sizeof(float);
				break;

			case DXGI_FORMAT_R16G16B16A16_TYPELESS:
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
			case DXGI_FORMAT_R16G16B16A16_UNORM:
			case DXGI_FORMAT_R16G16B16A16_UINT:
			case DXGI_FORMAT_R16G16B16A16_SNORM:
			case DXGI_FORMAT_R16G16B16A16_SINT:
				size += 4 * sizeof(short);
				break;

			case DXGI_FORMAT_R32G32_TYPELESS:
			case DXGI_FORMAT_R32G32_FLOAT:
			case DXGI_FORMAT_R32G32_UINT:
			case DXGI_FORMAT_R32G32_SINT:
				size += 2 * sizeof(float);
				break;

			case DXGI_FORMAT_R32G8X24_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
			case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
				size += 2 * sizeof(float);
				break;

			case DXGI_FORMAT_R10G10B10A2_TYPELESS:
			case DXGI_FORMAT_R10G10B10A2_UNORM:
			case DXGI_FORMAT_R10G10B10A2_UINT:
			case DXGI_FORMAT_R11G11B10_FLOAT:
			case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			case DXGI_FORMAT_R8G8B8A8_UINT:
			case DXGI_FORMAT_R8G8B8A8_SNORM:
			case DXGI_FORMAT_R8G8B8A8_SINT:
				size += 4 * sizeof(char);
				break;

			case DXGI_FORMAT_R16G16_TYPELESS:
			case DXGI_FORMAT_R16G16_FLOAT:
			case DXGI_FORMAT_R16G16_UNORM:
			case DXGI_FORMAT_R16G16_UINT:
			case DXGI_FORMAT_R16G16_SNORM:
			case DXGI_FORMAT_R16G16_SINT:
				size += 2 * sizeof(short);
				break;

			case DXGI_FORMAT_R32_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT_R32_FLOAT:
			case DXGI_FORMAT_R32_UINT:
			case DXGI_FORMAT_R32_SINT:
				size += 1 * sizeof(float);
				break;

			case DXGI_FORMAT_R24G8_TYPELESS:
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
			case DXGI_FORMAT_R8G8_B8G8_UNORM:
			case DXGI_FORMAT_G8R8_G8B8_UNORM:
			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8X8_UNORM:
				size += 4 * sizeof(char);
				break;

			case DXGI_FORMAT_R8G8_TYPELESS:
			case DXGI_FORMAT_R8G8_UNORM:
			case DXGI_FORMAT_R8G8_UINT:
			case DXGI_FORMAT_R8G8_SNORM:
			case DXGI_FORMAT_R8G8_SINT:
				size += 2 * sizeof(char);
				break;

			case DXGI_FORMAT_R16_TYPELESS:
			case DXGI_FORMAT_R16_FLOAT:
			case DXGI_FORMAT_D16_UNORM:
			case DXGI_FORMAT_R16_UNORM:
			case DXGI_FORMAT_R16_UINT:
			case DXGI_FORMAT_R16_SNORM:
			case DXGI_FORMAT_R16_SINT:
				size += 1 * sizeof(short);
				break;

			case DXGI_FORMAT_B5G6R5_UNORM:
			case DXGI_FORMAT_B5G5R5A1_UNORM:
				size += 1 * sizeof(short);
				break;

			case DXGI_FORMAT_R8_TYPELESS:
			case DXGI_FORMAT_R8_UNORM:
			case DXGI_FORMAT_R8_UINT:
			case DXGI_FORMAT_R8_SNORM:
			case DXGI_FORMAT_R8_SINT:
			case DXGI_FORMAT_A8_UNORM:
				size += 1 * sizeof(char);
				break;

			case DXGI_FORMAT_R1_UNORM:
				size += 1 * sizeof(char);
				break;
			}
	}

	return size;
}
