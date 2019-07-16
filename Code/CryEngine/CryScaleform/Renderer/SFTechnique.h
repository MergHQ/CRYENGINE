// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Renderer/SFConfig.h"
#include "Renderer/SFShaderDescs.h"
#include <Render/Render_Shader.h>

namespace Scaleform {
namespace Render {

using Scaleform::Render::D3D1x::VertexShaderDesc;
using Scaleform::Render::D3D1x::FragShaderDesc;
using Scaleform::Render::D3D1x::ShaderDesc;
using Scaleform::Render::D3D1x::Uniform;

class CSFRenderer;
class CSFTexture;

struct SVertexShaderOffsets
{
	SVertexShaderOffsets() : pInfo(nullptr) {}
	void CalculateOffsets(const VertexShaderDesc* pDesc);

	const VertexShaderDesc* pInfo;
	int                     offsets[Uniform::SU_Count];
};

struct SPixelShaderOffsets
{
	SPixelShaderOffsets() : pInfo(nullptr) {}
	void CalculateOffsets(const FragShaderDesc* pDesc);

	const FragShaderDesc* pInfo;
	int                   offsets[Uniform::SU_Count];
};

struct SSFTechnique
{
	typedef ShaderDesc::ShaderType ShaderType;

	SSFTechnique() : pVS(nullptr), pPS(nullptr), pVDesc(nullptr), pFDesc(nullptr), pVFormat(0) {}

	const SSFTechnique* operator->() const { return this; }
	operator bool() const { return pVS && pPS && pVS->pInfo && pPS->pInfo && pVFormat; }

	SVertexShaderOffsets*   pVS;
	SPixelShaderOffsets*    pPS;
	const VertexShaderDesc* pVDesc;
	const FragShaderDesc*   pFDesc;
	const VertexFormat*     pVFormat;
};

class CSFVertexLayout : public Render::SystemVertexFormat
{
public:
	CSFVertexLayout(const VertexFormat* vf, const VertexShaderDesc* vdesc);

	static const uint32 s_maxVertexElements = 8;
	InputLayoutHandle   hVertexLayout;
};

class CSFTechniqueInterface : public ShaderInterfaceBase<Uniform, SSFTechnique>
{
public:
	typedef const SSFTechnique Shader;

	CSFTechniqueInterface(Render::HAL* pRenderer) : m_pHal((CSFRenderer*)pRenderer) {}

	const Shader& GetCurrentShaders() const { return m_currentShaders; }
	bool          SetStaticShader(ShaderDesc::ShaderType shader, const VertexFormat* pvf);
	void          SetTexture(const Shader&, unsigned stage, Render::Texture* pTexture, ImageFillMode fm, unsigned index = 0);
	void          Finish(unsigned meshCount);

private:
	CSFRenderer* m_pHal;
	SSFTechnique m_currentShaders;
};

class CSFTechniqueManager : public StaticShaderManager<ShaderDesc, VertexShaderDesc, Uniform, CSFTechniqueInterface, CSFTexture>
{
	friend class CSFTechniqueInterface;
public:
	typedef StaticShaderManager<ShaderDesc, VertexShaderDesc, Uniform, CSFTechniqueInterface, CSFTexture> Base;
	typedef Uniform                                                                                       UniformType;

	CSFTechniqueManager(Render::HAL* pRenderer) : StaticShaderManager(pRenderer) {}

	void MapVertexFormat(PrimitiveFillType fill,
	                     const VertexFormat* sourceFormat,
	                     const VertexFormat** single,
	                     const VertexFormat** batch,
	                     const VertexFormat** instanced);

	bool            HasInstancingSupport() const { return true; }
	static unsigned GetDrawableImageFlags()      { return 0; }

private:
	SPixelShaderOffsets  m_pixelShaderInfos[FragShaderDesc::FSI_Count];
	SVertexShaderOffsets m_vertexShaderInfos[VertexShaderDesc::VSI_Count];
};

} // ~Render namespace
} // ~Scaleform namespace
