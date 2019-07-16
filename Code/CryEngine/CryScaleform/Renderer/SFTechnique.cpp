// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "Renderer/SFTechnique.h"
#include "Renderer/SFRenderer.h"
#include <Kernel/SF_Debug.h>

namespace Scaleform {
namespace Render {

namespace
{
struct SCpuConstantBuffer
{
	SCpuConstantBuffer() : size(0) {}

	void Update(float* inData, int inOffset, int inSize, int shadowLocation)
	{
		size = std::max(size, buffer_size_t(inOffset + inSize * sizeof(float)));
		memcpy(((UByte*)data) + inOffset, inData + shadowLocation, inSize * sizeof(float));
	}

	buffer_size_t size;
	float         data[Uniform::SU_VertexSize];
};
}

void SVertexShaderOffsets::CalculateOffsets(const VertexShaderDesc* pDesc)
{
	pInfo = pDesc;
	for (unsigned uniform = 0; uniform < Uniform::SU_Count; ++uniform)
	{
		offsets[uniform] = pDesc->Uniforms[uniform].Location * 16;
	}
}

void SPixelShaderOffsets::CalculateOffsets(const FragShaderDesc* pDesc)
{
	pInfo = pDesc;
	for (unsigned uniform = 0; uniform < Uniform::SU_Count; ++uniform)
	{
		offsets[uniform] = pDesc->Uniforms[uniform].Location * 16;
	}
}

CSFVertexLayout::CSFVertexLayout(const VertexFormat* vf, const VertexShaderDesc* pVDesc)
{
	D3D11_INPUT_ELEMENT_DESC elements[s_maxVertexElements];
	const VertexShaderDesc& vDesc = *pVDesc;
	UINT offset = 0;
	for (int attr = 0; attr < vDesc.NumAttribs; attr++)
	{
		D3D11_INPUT_ELEMENT_DESC& e = elements[attr];
		e.SemanticName = vDesc.Attributes[attr].SemanticName;
		e.SemanticIndex = vDesc.Attributes[attr].SemanticIndex;
		e.Format = (D3DFormat)vDesc.Attributes[attr].Format;
		e.InputSlot = 0;
		e.AlignedByteOffset = offset;
		e.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		e.InstanceDataStepRate = 0;

		offset += DeviceFormats::GetStride(e.Format);
	}

	hVertexLayout = CDeviceObjectFactory::CreateCustomVertexFormat(vDesc.NumAttribs, elements);
}

bool CSFTechniqueInterface::SetStaticShader(ShaderDesc::ShaderType shaderType, const VertexFormat* pFormat)
{
	VertexShaderDesc::ShaderIndex vsi = VertexShaderDesc::GetShaderIndex(shaderType, ShaderDesc::ShaderVersion_Default);
	FragShaderDesc::ShaderIndex fsi = FragShaderDesc::GetShaderIndex(shaderType, ShaderDesc::ShaderVersion_Default);

	m_currentShaders.pVFormat = pFormat;
	m_currentShaders.pVS = &m_pHal->SManager.m_vertexShaderInfos[vsi];
	m_currentShaders.pPS = &m_pHal->SManager.m_pixelShaderInfos[fsi];

	if (!m_currentShaders->pVS->pInfo)
	{
		m_currentShaders.pVS->CalculateOffsets(VertexShaderDesc::Descs[vsi]);
	}

	if (!m_currentShaders->pPS->pInfo)
	{
		m_currentShaders.pPS->CalculateOffsets(FragShaderDesc::Descs[fsi]);
	}

	m_currentShaders.pVDesc = m_currentShaders->pVS->pInfo;
	m_currentShaders.pFDesc = m_currentShaders->pPS->pInfo;

	if (pFormat && !pFormat->pSysFormat)
	{
		(const_cast<VertexFormat*>(pFormat))->pSysFormat = *SF_NEW CSFVertexLayout(pFormat, m_currentShaders.pVS->pInfo);
	}
	return (bool)m_currentShaders;
}

void CSFTechniqueInterface::SetTexture(const Shader&, unsigned var, Render::Texture* pTexture, ImageFillMode fm, unsigned index)
{
	CSFTexture* pd3dTexture = (CSFTexture*)pTexture;
	CRY_ASSERT(m_currentShaders.pFDesc->Uniforms[var].Location >= 0);
	CRY_ASSERT(m_currentShaders.pFDesc->Uniforms[var].Location + m_currentShaders.pFDesc->Uniforms[var].Size >= (short)(index + pTexture->TextureCount));
	pd3dTexture->ApplyTexture(m_currentShaders.pFDesc->Uniforms[var].Location + index, fm);
}

void CSFTechniqueInterface::Finish(unsigned meshCount)
{
	ShaderInterfaceBase::Finish(meshCount);
	SCpuConstantBuffer pixelConstantBuffer;
	SCpuConstantBuffer vertexConstantBuffer;

	for (int i = 0; i < Uniform::SU_Count; i++)
	{
		if (UniformSet[i])
		{
			if (0 <= m_currentShaders.pPS->offsets[i])
			{
				pixelConstantBuffer.Update(
					UniformData,
					m_currentShaders.pPS->offsets[i],
					m_currentShaders.pFDesc->Uniforms[i].Size,
					m_currentShaders.pFDesc->Uniforms[i].ShadowOffset);
			}
			else if (0 <= m_currentShaders.pVS->offsets[i])
			{
				vertexConstantBuffer.Update(
					UniformData,
					m_currentShaders.pVS->offsets[i],
					m_currentShaders.pVDesc->Uniforms[i].Size,
					m_currentShaders.pVDesc->Uniforms[i].ShadowOffset);
			}
		}
	}

	m_pHal->SetVertexShaderConstants(vertexConstantBuffer.data, vertexConstantBuffer.size);
	m_pHal->SetPixelShaderConstants(pixelConstantBuffer.data, pixelConstantBuffer.size);
	memset(UniformSet, 0, sizeof(UniformSet));
}

void CSFTechniqueManager::MapVertexFormat(PrimitiveFillType fill, const VertexFormat* sourceFormat,
                                          const VertexFormat** single, const VertexFormat** batch,
                                          const VertexFormat** instanced)
{
	VertexElement floatPositionElements[8];
	VertexFormat floatPositionFormat;
	floatPositionFormat.pElements = floatPositionElements;
	floatPositionFormat.pSysFormat = 0;
	uint32 i = 0;
	for (; sourceFormat->pElements[i].Attribute != VET_None; ++i)
	{
		floatPositionElements[i].Attribute = sourceFormat->pElements[i].Attribute;
		floatPositionElements[i].Offset = sourceFormat->pElements[i].Offset;
		if (VET_Pos == (floatPositionElements[i].Attribute & VET_Usage_Mask))
		{
			floatPositionElements[i].Attribute &= ~VET_CompType_Mask;
			floatPositionElements[i].Attribute |= VET_F32;
		}
	}
	floatPositionElements[i].Attribute = VET_None;
	floatPositionElements[i].Offset = 0;

	Base::MapVertexFormat(fill, &floatPositionFormat, single, batch, instanced, MVF_HasInstancing | MVF_Align);
}

} // ~Render namespace
} // ~Scaleform namespace
