// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved. 

#include "StdAfx.h"
#include "DeviceObjects.h"
#include "DriverD3D.h"
#include "xxhash.h"
#include "Common/ReverseDepth.h"
#include "Common/Textures/TextureHelpers.h"
#include "../GraphicsPipeline/Common/GraphicsPipelineStateSet.h"

CDeviceObjectFactory CDeviceObjectFactory::m_singleton;

CDeviceObjectFactory::~CDeviceObjectFactory()
{
	if (m_fence_handle != DeviceFenceHandle() && FAILED(ReleaseFence(m_fence_handle)))
	{
		CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, "could not release sync fence");
	}
}

bool CDeviceObjectFactory::CanUseCoreCommandList()
{
	return gcpRendD3D->m_pRT->IsRenderThread() && !gcpRendD3D->m_pRT->IsRenderLoadingThread();
}

////////////////////////////////////////////////////////////////////////////
// Fence API (TODO: offload all to CDeviceFenceHandle)

void CDeviceObjectFactory::SyncToGPU()
{
	if (CRenderer::CV_r_enable_full_gpu_sync)
	{
		if (m_fence_handle == DeviceFenceHandle() && FAILED(CreateFence(m_fence_handle)))
		{
			CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, "could not create sync fence");
		}
		if (m_fence_handle)
		{
			IssueFence(m_fence_handle);
			SyncFence(m_fence_handle, true);
		}
	}
}

void CDeviceObjectFactory::IssueFrameFences()
{
	static_assert(CRY_ARRAY_COUNT(m_frameFences) == MAX_FRAMES_IN_FLIGHT, "Unexpected size for m_frameFences");

	if (!m_frameFences[0])
	{
		m_frameFenceCounter = MAX_FRAMES_IN_FLIGHT;
		for (uint32 i = 0; i < CRY_ARRAY_COUNT(m_frameFences); i++)
		{
			HRESULT hr = CreateFence(m_frameFences[i]);
			assert(hr == S_OK);
		}
		return;
	}

	HRESULT hr = IssueFence(m_frameFences[m_frameFenceCounter % MAX_FRAMES_IN_FLIGHT]);
	assert(hr == S_OK);

	if (CRenderer::CV_r_SyncToFrameFence)
	{
		// Stall render thread until GPU has finished processing previous frame (in case max frame latency is 1)
		PROFILE_FRAME("WAIT FOR GPU");
		HRESULT hr = SyncFence(m_frameFences[(m_frameFenceCounter - (MAX_FRAMES_IN_FLIGHT - 1)) % MAX_FRAMES_IN_FLIGHT], true, true);
		assert(hr == S_OK);
	}
	m_completedFrameFenceCounter = m_frameFenceCounter - (MAX_FRAMES_IN_FLIGHT - 1);
	m_frameFenceCounter += 1;
}

void CDeviceObjectFactory::ReleaseFrameFences()
{
	for (uint32 i = 0; i < CRY_ARRAY_COUNT(m_frameFences); i++)
		ReleaseFence(m_frameFences[i]);
}

////////////////////////////////////////////////////////////////////////////
// SamplerState API

CStaticDeviceObjectStorage<SamplerStateHandle, SSamplerState, CDeviceSamplerState, false, CDeviceObjectFactory::CreateSamplerState> CDeviceObjectFactory::s_SamplerStates;

void CDeviceObjectFactory::AllocatePredefinedSamplerStates()
{
	ReserveSamplerStates(300); // this likes to expand, so it'd be nice if it didn't; 300 => ~6Kb, there were 171 after one level

	// *INDENT-OFF*
	SamplerStateHandle a = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_POINT,     eSamplerAddressMode_Clamp,  eSamplerAddressMode_Clamp,  eSamplerAddressMode_Clamp,   0      )); assert(a == EDefaultSamplerStates::PointClamp           );
	SamplerStateHandle b = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_POINT,     eSamplerAddressMode_Wrap,   eSamplerAddressMode_Wrap,   eSamplerAddressMode_Wrap,    0      )); assert(b == EDefaultSamplerStates::PointWrap            );
	SamplerStateHandle c = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_POINT,     eSamplerAddressMode_Border, eSamplerAddressMode_Border, eSamplerAddressMode_Border,  0      )); assert(c == EDefaultSamplerStates::PointBorder_Black    );
	SamplerStateHandle d = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_POINT,     eSamplerAddressMode_Border, eSamplerAddressMode_Border, eSamplerAddressMode_Border, ~0      )); assert(d == EDefaultSamplerStates::PointBorder_White    );
	SamplerStateHandle e = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_POINT,     eSamplerAddressMode_Wrap,   eSamplerAddressMode_Wrap,   eSamplerAddressMode_Wrap,    0, true)); assert(e == EDefaultSamplerStates::PointCompare         );
	SamplerStateHandle f = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_LINEAR,    eSamplerAddressMode_Clamp,  eSamplerAddressMode_Clamp,  eSamplerAddressMode_Clamp,   0      )); assert(f == EDefaultSamplerStates::LinearClamp          );
	SamplerStateHandle g = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_LINEAR,    eSamplerAddressMode_Wrap,   eSamplerAddressMode_Wrap,   eSamplerAddressMode_Wrap,    0      )); assert(g == EDefaultSamplerStates::LinearWrap           );
	SamplerStateHandle h = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_LINEAR,    eSamplerAddressMode_Border, eSamplerAddressMode_Border, eSamplerAddressMode_Border,  0      )); assert(h == EDefaultSamplerStates::LinearBorder_Black   );
	SamplerStateHandle i = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_LINEAR,    eSamplerAddressMode_Clamp,  eSamplerAddressMode_Clamp,  eSamplerAddressMode_Clamp,   0, true)); assert(i == EDefaultSamplerStates::LinearCompare        );
	SamplerStateHandle j = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_BILINEAR,  eSamplerAddressMode_Clamp,  eSamplerAddressMode_Clamp,  eSamplerAddressMode_Clamp,   0      )); assert(j == EDefaultSamplerStates::BilinearClamp        );
	SamplerStateHandle k = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_BILINEAR,  eSamplerAddressMode_Wrap,   eSamplerAddressMode_Wrap,   eSamplerAddressMode_Wrap,    0      )); assert(k == EDefaultSamplerStates::BilinearWrap         );
	SamplerStateHandle l = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_BILINEAR,  eSamplerAddressMode_Border, eSamplerAddressMode_Border, eSamplerAddressMode_Border,  0      )); assert(l == EDefaultSamplerStates::BilinearBorder_Black );
	SamplerStateHandle m = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_BILINEAR,  eSamplerAddressMode_Wrap,   eSamplerAddressMode_Wrap,   eSamplerAddressMode_Wrap,    0, true)); assert(m == EDefaultSamplerStates::BilinearCompare      );
	SamplerStateHandle n = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_TRILINEAR, eSamplerAddressMode_Clamp,  eSamplerAddressMode_Clamp,  eSamplerAddressMode_Clamp,   0      )); assert(n == EDefaultSamplerStates::TrilinearClamp       );
	SamplerStateHandle o = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_TRILINEAR, eSamplerAddressMode_Wrap,   eSamplerAddressMode_Wrap,   eSamplerAddressMode_Wrap,    0      )); assert(o == EDefaultSamplerStates::TrilinearWrap        );
	SamplerStateHandle p = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_TRILINEAR, eSamplerAddressMode_Border, eSamplerAddressMode_Border, eSamplerAddressMode_Border,  0      )); assert(p == EDefaultSamplerStates::TrilinearBorder_Black);
	SamplerStateHandle q = GetOrCreateSamplerStateHandle(SSamplerState(FILTER_TRILINEAR, eSamplerAddressMode_Border, eSamplerAddressMode_Border, eSamplerAddressMode_Border, ~0      )); assert(q == EDefaultSamplerStates::TrilinearBorder_White);
	// *INDENT-ON*
}

void CDeviceObjectFactory::TrimSamplerStates()
{
	s_SamplerStates.Release(EDefaultSamplerStates::PreAllocated);
}

void CDeviceObjectFactory::ReleaseSamplerStates()
{
	s_SamplerStates.Release(EDefaultSamplerStates::PointClamp);
}

////////////////////////////////////////////////////////////////////////////
// InputLayout API

CStaticDeviceObjectStorage<InputLayoutHandle, SInputLayout, CDeviceInputLayout, true, CDeviceObjectFactory::CreateInputLayout> CDeviceObjectFactory::s_InputLayouts;

std::vector<InputLayoutHandle> CDeviceObjectFactory::s_InputLayoutPermutations[1 << VSF_NUM][3]; // [StreamMask][Morph][VertexFmt]

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_Empty[] = { {} }; // Empty

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_P3F_C4B_T2F[] = 
{
	{ "POSITION"    , 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, offsetof(SVF_P3F_C4B_T2F     , xyz  ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"       , 0, DXGI_FORMAT_R8G8B8A8_UNORM    , 0, offsetof(SVF_P3F_C4B_T2F     , color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"    , 0, DXGI_FORMAT_R32G32_FLOAT      , 0, offsetof(SVF_P3F_C4B_T2F     , st   ), D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_P3S_C4B_T2S[] =
{
	{ "POSITION"    , 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, offsetof(SVF_P3S_C4B_T2S     , xyz  ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"       , 0, DXGI_FORMAT_R8G8B8A8_UNORM    , 0, offsetof(SVF_P3S_C4B_T2S     , color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"    , 0, DXGI_FORMAT_R16G16_FLOAT      , 0, offsetof(SVF_P3S_C4B_T2S     , st   ), D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_P3S_N4B_C4B_T2S[] =
{
	{ "POSITION"    , 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, offsetof(SVF_P3S_N4B_C4B_T2S, xyz   ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL"      , 0, DXGI_FORMAT_R8G8B8A8_UNORM    , 0, offsetof(SVF_P3S_N4B_C4B_T2S, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"       , 0, DXGI_FORMAT_R8G8B8A8_UNORM    , 0, offsetof(SVF_P3S_N4B_C4B_T2S, color ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"    , 0, DXGI_FORMAT_R16G16_FLOAT      , 0, offsetof(SVF_P3S_N4B_C4B_T2S, st    ), D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_P3F_C4B_T4B_N3F2[] = // ParticleVT.cfi: app2vertParticleGeneral
{
	{ "POSITION"    , 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, offsetof(SVF_P3F_C4B_T4B_N3F2, xyz  ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"       , 0, DXGI_FORMAT_R8G8B8A8_UNORM    , 0, offsetof(SVF_P3F_C4B_T4B_N3F2, color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"    , 0, DXGI_FORMAT_R8G8B8A8_UNORM    , 0, offsetof(SVF_P3F_C4B_T4B_N3F2, st   ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "AXIS"        , 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, offsetof(SVF_P3F_C4B_T4B_N3F2, xaxis), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "AXIS"        , 1, DXGI_FORMAT_R32G32B32_FLOAT   , 0, offsetof(SVF_P3F_C4B_T4B_N3F2, yaxis), D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_TP3F_C4B_T2F[] =
{
	{ "POSITION"    , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(SVF_TP3F_C4B_T2F    , pos  ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"       , 0, DXGI_FORMAT_R8G8B8A8_UNORM    , 0, offsetof(SVF_TP3F_C4B_T2F    , color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"    , 0, DXGI_FORMAT_R32G32_FLOAT      , 0, offsetof(SVF_TP3F_C4B_T2F    , st   ), D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_P3F_T3F[] =
{
	{ "POSITION"    , 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, offsetof(SVF_P3F_T3F         , p    ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"    , 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, offsetof(SVF_P3F_T3F         , st   ), D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_P3F_T2F_T3F[] =
{
	{ "POSITION"    , 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, offsetof(SVF_P3F_T2F_T3F     , p    ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"    , 0, DXGI_FORMAT_R32G32_FLOAT      , 0, offsetof(SVF_P3F_T2F_T3F     , st0  ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"    , 1, DXGI_FORMAT_R32G32B32_FLOAT   , 0, offsetof(SVF_P3F_T2F_T3F     , st1  ), D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_P3F[] =
{
	{ "POSITION"    , 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, offsetof(SVF_P3F             , xyz  ), D3D11_INPUT_PER_VERTEX_DATA, 0 },

	// TODO: remove this from the definition, it's a workaround for ZPass shaders expecting COLOR/TEXCOORD
	{ "COLOR"       , 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, offsetof(SVF_P3F             , xyz  ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"    , 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, offsetof(SVF_P3F             , xyz  ), D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_P2S_N4B_C4B_T1F[] =
{
	{ "POSITION"    , 0, DXGI_FORMAT_R16G16_FLOAT      , 0, offsetof(SVF_P2S_N4B_C4B_T1F, xy    ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL"      , 0, DXGI_FORMAT_R8G8B8A8_UNORM    , 0, offsetof(SVF_P2S_N4B_C4B_T1F, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"       , 0, DXGI_FORMAT_R8G8B8A8_UNORM    , 0, offsetof(SVF_P2S_N4B_C4B_T1F, color ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"    , 0, DXGI_FORMAT_R32_FLOAT         , 0, offsetof(SVF_P2S_N4B_C4B_T1F, z     ), D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_P3F_C4B_T2S[] =
{
	{ "POSITION"    , 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, offsetof(SVF_P3F_C4B_T2S     , xyz  ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"       , 0, DXGI_FORMAT_R8G8B8A8_UNORM    , 0, offsetof(SVF_P3F_C4B_T2S     , color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"    , 0, DXGI_FORMAT_R16G16_FLOAT      , 0, offsetof(SVF_P3F_C4B_T2S     , st   ), D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_T4F_B4F[] =
{
	{ "TANGENT"     , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, VSF_TANGENTS,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BITANGENT"   , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, VSF_TANGENTS, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_T4S_B4S[] =
{
	{ "TANGENT"     , 0, DXGI_FORMAT_R16G16B16A16_SNORM, VSF_TANGENTS,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BITANGENT"   , 0, DXGI_FORMAT_R16G16B16A16_SNORM, VSF_TANGENTS,  8, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_Q4F[] =
{
	{ "TANGENT"     , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, VSF_QTANGENTS, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_Q4S[] =
{
	{ "TANGENT"     , 0, DXGI_FORMAT_R16G16B16A16_SNORM, VSF_QTANGENTS, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_N3F[] =
{
	{ "NORMAL"      , 0, DXGI_FORMAT_R32G32B32_FLOAT   , VSF_NORMALS  , 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_W4B_I4S[] =
{
	{ "BLENDWEIGHT" , 0, DXGI_FORMAT_R8G8B8A8_UNORM    , VSF_HWSKIN_INFO, offsetof(SVF_W4B_I4S, weights), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BLENDINDICES", 0, DXGI_FORMAT_R16G16B16A16_SINT , VSF_HWSKIN_INFO, offsetof(SVF_W4B_I4S, indices), D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_V3F[] =
{
	{ "POSITION"    , 3, DXGI_FORMAT_R32G32B32_FLOAT   , VSF_VERTEX_VELOCITY, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_W2F[] =
{
	{ "BLENDWEIGHT" , 1, DXGI_FORMAT_R32G32_FLOAT      , VSF_MORPHBUDDY_WEIGHTS, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static const D3D11_INPUT_ELEMENT_DESC VertexDecl_V4Fi[] =
{
	{ "TEXCOORD"    , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
};

static const struct
{
	size_t numDescs;
	const D3D11_INPUT_ELEMENT_DESC* inputDescs;
}
VertexDecls[EDefaultInputLayouts::PreAllocated] =
{
	{ 0, VertexDecl_Empty            },

	// Base stream
	{ CRY_ARRAY_COUNT(VertexDecl_P3F_C4B_T2F     ), VertexDecl_P3F_C4B_T2F      },
	{ CRY_ARRAY_COUNT(VertexDecl_P3S_C4B_T2S     ), VertexDecl_P3S_C4B_T2S      },
	{ CRY_ARRAY_COUNT(VertexDecl_P3S_N4B_C4B_T2S ), VertexDecl_P3S_N4B_C4B_T2S  },

	{ CRY_ARRAY_COUNT(VertexDecl_P3F_C4B_T4B_N3F2), VertexDecl_P3F_C4B_T4B_N3F2 },
	{ CRY_ARRAY_COUNT(VertexDecl_TP3F_C4B_T2F    ), VertexDecl_TP3F_C4B_T2F     },
	{ CRY_ARRAY_COUNT(VertexDecl_P3F_T3F         ), VertexDecl_P3F_T3F          },
	{ CRY_ARRAY_COUNT(VertexDecl_P3F_T2F_T3F     ), VertexDecl_P3F_T2F_T3F      },

	{ CRY_ARRAY_COUNT(VertexDecl_P3F             ), VertexDecl_P3F              },

	{ CRY_ARRAY_COUNT(VertexDecl_P2S_N4B_C4B_T1F ), VertexDecl_P2S_N4B_C4B_T1F  },
	{ CRY_ARRAY_COUNT(VertexDecl_P3F_C4B_T2S     ), VertexDecl_P3F_C4B_T2S      },

	// Additional streams
	{ CRY_ARRAY_COUNT(VertexDecl_T4F_B4F         ), VertexDecl_T4F_B4F          },
	{ CRY_ARRAY_COUNT(VertexDecl_T4S_B4S         ), VertexDecl_T4S_B4S          },
	{ CRY_ARRAY_COUNT(VertexDecl_Q4F             ), VertexDecl_Q4F              },
	{ CRY_ARRAY_COUNT(VertexDecl_Q4S             ), VertexDecl_Q4S              },
	{ CRY_ARRAY_COUNT(VertexDecl_N3F             ), VertexDecl_N3F              },
	{ CRY_ARRAY_COUNT(VertexDecl_W4B_I4S         ), VertexDecl_W4B_I4S          },
	{ CRY_ARRAY_COUNT(VertexDecl_V3F             ), VertexDecl_V3F              },
	{ CRY_ARRAY_COUNT(VertexDecl_W2F             ), VertexDecl_W2F              },

	{ CRY_ARRAY_COUNT(VertexDecl_V4Fi            ), VertexDecl_V4Fi             }
};

void CDeviceObjectFactory::AllocatePredefinedInputLayouts()
{
	ReserveInputLayouts(EDefaultInputLayouts::PreAllocated);

	for (InputLayoutHandle nFormat = EDefaultInputLayouts::Empty; nFormat < EDefaultInputLayouts::PreAllocated; nFormat = InputLayoutHandle::ValueType(uint16(nFormat) + 1U))
	{
		SInputLayout out;
		for (size_t n = 0; n < VertexDecls[uint32(nFormat)].numDescs; n++)
			out.m_Declaration.push_back(VertexDecls[uint32(nFormat)].inputDescs[n]);

		// General vertex stream stride
		out.CalculateStride();
		out.CalculateOffsets();
		out.m_pVertexShader = nullptr;

		InputLayoutHandle r = GetOrCreateInputLayoutHandle(out);
		assert(r == nFormat);
	}

	for (int n = 0; n <= MASK(VSF_NUM); ++n)
	{
		s_InputLayoutPermutations[n][0].resize(EDefaultInputLayouts::PreAllocated);
		s_InputLayoutPermutations[n][1].resize(EDefaultInputLayouts::PreAllocated);
	}

	// NOTE: s_InputLayoutPermutations is entirely empty in the beginning. Subsequent calls to
	// GetOrCreateInputLayoutHandle() with a valid shader blob will retry creating the input layout objects
}

void CDeviceObjectFactory::TrimInputLayouts()
{
	s_InputLayouts.Release(EDefaultInputLayouts::PreAllocated);

	// for simplicity sake, clear lookup table
	for (auto& permutationsForMask : s_InputLayoutPermutations)
		for (auto& permutationsForMorph : permutationsForMask)
			permutationsForMorph.clear();
}

void CDeviceObjectFactory::ReleaseInputLayouts()
{
	s_InputLayouts.Release(EDefaultInputLayouts::Empty);

	// for simplicity sake, clear lookup table
	for (auto& permutationsForMask : s_InputLayoutPermutations)
		for (auto& permutationsForMorph : permutationsForMask)
			permutationsForMorph.clear();
}

InputLayoutHandle CDeviceObjectFactory::GetOrCreateInputLayoutHandle(const SShaderBlob* pVertexShader, int StreamMask, int InstAttrMask, uint32 nUsedAttr, byte Attributes[], const InputLayoutHandle VertexFormat)
{
	bool bMorph = (StreamMask & VSM_MORPHBUDDY) != 0;
	bool bInstanced = (StreamMask & VSM_INSTANCED) != 0;
	bool bInstAttribs = (InstAttrMask != 0);

	int nShared = (bInstAttribs) ? 2 : ((bMorph || bInstanced) ? 1 : 0);
	InputLayoutHandle *pDeclCache = &s_InputLayoutPermutations[StreamMask % MASK(VSF_NUM)][nShared][VertexFormat];

#if CRY_PLATFORM_ORBIS && !defined(CUSTOM_FETCH_SHADERS)
	// TODO: Discuss with PS4 implementer!

	// Orbis shader compiler strips out unused declarations so keep creating until we find a shader that's fully bound
	if (pDeclCache->m_pDeclaration && !pDeclCache->m_pDeclaration->IsFullyBound())
	{
		SAFE_RELEASE(pDeclCache->m_pDeclaration);
	}
#endif

	if (*pDeclCache == InputLayoutHandle::Unspecified)
	{
		*pDeclCache = VertexFormat;

		//	iLog->Log("OnDemandVertexDeclaration %d %d %d (DEBUG test - shouldn't log too often)", StreamMask, VertexFormat, bMorph ? 1 : 0);
		const std::pair<SInputLayout, CDeviceInputLayout*>& baseLayout = LookupInputLayout(VertexFormat);

		if (baseLayout.first.m_pVertexShader != pVertexShader)
		{
			SInputLayout out = baseLayout.first;

			if (StreamMask || bInstAttribs)
			{
				// Create instanced vertex declaration
				if (bInstanced)
				{
					for (size_t n = 0; n < out.m_Declaration.size(); n++)
					{
						D3D11_INPUT_ELEMENT_DESC& elem = out.m_Declaration[n];

						elem.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
						elem.InstanceDataStepRate = 1;
					}
				}

				// Append additional streams
				for (int n = 1; n < VSF_NUM; n++)
				{
					if (!(StreamMask & (1 << n)))
						continue;

					InputLayoutHandle AttachmentFormat = EDefaultInputLayouts::Unspecified;
					switch (n)
					{
					#ifdef TANG_FLOATS
						case VSF_TANGENTS       : AttachmentFormat = EDefaultInputLayouts::T4F_B4F; break;
						case VSF_QTANGENTS      : AttachmentFormat = EDefaultInputLayouts::Q4F; break;
					#else
						case VSF_TANGENTS       : AttachmentFormat = EDefaultInputLayouts::T4S_B4S; break;
						case VSF_QTANGENTS      : AttachmentFormat = EDefaultInputLayouts::Q4S; break;
					#endif
						case VSF_HWSKIN_INFO    : AttachmentFormat = EDefaultInputLayouts::W4B_I4S; break;
						case VSF_VERTEX_VELOCITY: AttachmentFormat = EDefaultInputLayouts::V3F; break;
						case VSF_NORMALS        : AttachmentFormat = EDefaultInputLayouts::N3F; break;
					}

					const std::pair<SInputLayout, CDeviceInputLayout*>& addLayout = LookupInputLayout(AttachmentFormat);
					for (size_t n = 0; n < addLayout.first.m_Declaration.size(); n++)
						out.m_Declaration.push_back(addLayout.first.m_Declaration[n]);
				}

				// Append morph buddies
				if (bMorph)
				{
					uint32 dwNumWithoutMorph = out.m_Declaration.size();

					for (size_t n = 0; n < dwNumWithoutMorph; n++)
					{
						D3D11_INPUT_ELEMENT_DESC El = out.m_Declaration[n];

						El.InputSlot += VSF_MORPHBUDDY;
						El.SemanticIndex += 8;

						out.m_Declaration.push_back(El);
					}

					const std::pair<SInputLayout, CDeviceInputLayout*>& addLayout = LookupInputLayout(EDefaultInputLayouts::W2F);
					for (size_t n = 0; n < addLayout.first.m_Declaration.size(); n++)
						out.m_Declaration.push_back(addLayout.first.m_Declaration[n]);
				}

				// Append instance data
				if (bInstAttribs)
				{
					const int nInstOffs = 1;

					const std::pair<SInputLayout, CDeviceInputLayout*>& addLayout = LookupInputLayout(EDefaultInputLayouts::V4Fi);
					for (int i = 0; i < nUsedAttr; i++)
					{
						for (size_t n = 0; n < addLayout.first.m_Declaration.size(); n++)
						{
							D3D11_INPUT_ELEMENT_DESC El = addLayout.first.m_Declaration[n];

							El.AlignedByteOffset = i * addLayout.first.m_Stride;
							El.SemanticIndex = Attributes[i] + nInstOffs;

							out.m_Declaration.push_back(El);
						}
					}
				}
			}

			// General vertex stream stride
			out.CalculateStride();
			out.CalculateOffsets();
			out.m_pVertexShader = pVertexShader;

			*pDeclCache = GetOrCreateInputLayoutHandle(out);
			assert(*pDeclCache == VertexFormat || *pDeclCache >= EDefaultInputLayouts::PreAllocated);
		}
	}

	// NOTE: s_InputLayoutPermutations is entirely empty in the beginning. Subsequent calls to
	// GetOrCreateInputLayoutHandle() with a valid shader blob will retry creating the input layout objects

	assert(pVertexShader);
	assert(LookupInputLayout(*pDeclCache).first.m_pVertexShader);

	return *pDeclCache;
}

InputLayoutHandle CDeviceObjectFactory::GetOrCreateInputLayoutHandle(const SShaderBlob* pVertexShader, size_t numDescs, const D3D11_INPUT_ELEMENT_DESC* inputLayout)
{
	SInputLayout out;
	for (int n = 0; n < numDescs; ++n)
		out.m_Declaration.push_back(inputLayout[n]);

	// General vertex stream stride
	out.CalculateStride();
	out.CalculateOffsets();
	out.m_pVertexShader = pVertexShader;

	InputLayoutHandle newVF = GetOrCreateInputLayoutHandle(out);

	for (int n = 0; n <= MASK(VSF_NUM); ++n)
	{
		s_InputLayoutPermutations[n][0].resize(uint32(newVF) + 1U);
		s_InputLayoutPermutations[n][1].resize(uint32(newVF) + 1U);
	}

	return newVF;
}

void CDeviceObjectFactory::EraseRenderPass(CDeviceRenderPass* pPass, bool bRemoveInvalidateCallbacks)
{
	CryAutoCriticalSectionNoRecursive lock(m_RenderPassCacheLock);

	for (auto it = m_RenderPassCache.begin(), itEnd = m_RenderPassCache.end(); it != itEnd; ++it)
	{
		if (it->second.get() == pPass)
		{
			if (!bRemoveInvalidateCallbacks)
			{
				auto pPassDesc = const_cast<CDeviceRenderPassDesc*>(&it->first);
				
				pPassDesc->m_invalidateCallbackOwner = nullptr;
				pPassDesc->m_invalidateCallback = nullptr;
			}

			m_RenderPassCache.erase(it);
			break;
		}
	}
}

void CDeviceObjectFactory::TrimRenderPasses()
{
	CryAutoCriticalSectionNoRecursive lock(m_RenderPassCacheLock);

	EraseUnusedEntriesFromCache(m_RenderPassCache);
}

bool CDeviceObjectFactory::OnRenderPassInvalidated(void* pRenderPass, SResourceBindPoint bindPoint, UResourceReference pResource, uint32 flags)
{
	auto pPass     = reinterpret_cast<CDeviceRenderPass*>(pRenderPass);

	// Don't keep the pointer and unregister the callback when the resource goes out of scope
	if (flags & eResourceDestroyed)
	{
		GetDeviceObjectFactory().EraseRenderPass(pPass, false);
		return false;
	}

	if (flags)
	{
		if (auto pDesc = GetDeviceObjectFactory().GetRenderPassDesc(pPass))
		{
			pPass->Invalidate();
		}
	}

	return true;
}

void CDeviceObjectFactory::TrimResources()
{
	CRY_ASSERT(gRenDev->m_pRT->IsRenderThread());

	TrimPipelineStates();
	TrimResourceLayouts();
	TrimRenderPasses();

	for (auto& itLayout : m_ResourceLayoutCache)
	{
		const SDeviceResourceLayoutDesc& desc = itLayout.first;
		for (auto& itLayoutBinding : desc.m_resourceBindings)
		{
			for (auto& itResource : itLayoutBinding.second)
			{
				const SResourceBinding& resource = itResource.second;
				if (resource.type == SResourceBinding::EResourceType::Sampler)
				{
					if (resource.samplerState >= EDefaultSamplerStates::PreAllocated)
						goto trimSamplerStatesDone; // non-preallocated sampler state still in use. cannot trim sampler states at this point
				}
			}
		}
	}

	TrimSamplerStates();

trimSamplerStatesDone:

	bool bCustomInputLayoutsInUse = false;
	for (auto& itPso : m_GraphicsPsoCache)
	{
		const CDeviceGraphicsPSODesc& desc = itPso.first;
		if (desc.m_VertexFormat >= EDefaultInputLayouts::PreAllocated)
		{
			bCustomInputLayoutsInUse = true;
			break;
		}
	}

	if (!bCustomInputLayoutsInUse)
		TrimInputLayouts();
}

void CDeviceObjectFactory::ReleaseResources()
{
	m_GraphicsPsoCache.clear();
	m_InvalidGraphicsPsos.clear();

	m_ComputePsoCache.clear();
	m_InvalidComputePsos.clear();

	m_ResourceLayoutCache.clear();

	{
		CryAutoCriticalSectionNoRecursive lock(m_RenderPassCacheLock);
		m_RenderPassCache.clear();
	}

	ReleaseSamplerStates();
	ReleaseInputLayouts();

	// Unblock/unbind and free live resources
	ReleaseResourcesImpl();
}

void CDeviceObjectFactory::ReloadPipelineStates()
{
	for (auto it = m_GraphicsPsoCache.begin(), itEnd = m_GraphicsPsoCache.end(); it != itEnd; )
	{
		auto itCurrentPSO = it++;
		const CDeviceGraphicsPSO::EInitResult result = itCurrentPSO->second->Init(itCurrentPSO->first);

		if (result != CDeviceGraphicsPSO::EInitResult::Success)
		{
			switch (result)
			{
			case CDeviceGraphicsPSO::EInitResult::Failure:
				m_InvalidGraphicsPsos.emplace(itCurrentPSO->first, itCurrentPSO->second);
				break;
			case CDeviceGraphicsPSO::EInitResult::ErrorShadersAndTopologyCombination:
				m_GraphicsPsoCache.erase(itCurrentPSO);
				break;
			default:
				break;
			}
		}
	}

	for (auto& it : m_ComputePsoCache)
	{
		if (!it.second->Init(it.first))
			m_InvalidComputePsos.emplace(it.first, it.second);
	}
}

void CDeviceObjectFactory::UpdatePipelineStates()
{
	for (auto it = m_InvalidGraphicsPsos.begin(), itEnd = m_InvalidGraphicsPsos.end(); it != itEnd; )
	{
		auto itCurrentPSO = it++;
		auto pPso = itCurrentPSO->second.lock();

		if (!pPso)
		{
			m_InvalidGraphicsPsos.erase(itCurrentPSO);
		}
		else
		{
			const CDeviceGraphicsPSO::EInitResult result = pPso->Init(itCurrentPSO->first);
			if (result != CDeviceGraphicsPSO::EInitResult::Failure)
			{
				if (result == CDeviceGraphicsPSO::EInitResult::ErrorShadersAndTopologyCombination)
				{
					auto find = m_GraphicsPsoCache.find(itCurrentPSO->first);
					if (find != m_GraphicsPsoCache.end())
					{
						m_GraphicsPsoCache.erase(find);
					}
				}

				m_InvalidGraphicsPsos.erase(itCurrentPSO);
			}
		}
	}

	for (auto it = m_InvalidComputePsos.begin(), itEnd = m_InvalidComputePsos.end(); it != itEnd; )
	{
		auto itCurrentPSO = it++;
		auto pPso = itCurrentPSO->second.lock();

		if (!pPso || pPso->Init(itCurrentPSO->first))
			m_InvalidComputePsos.erase(itCurrentPSO);
	}
}

void CDeviceObjectFactory::TrimPipelineStates()
{
	EraseUnusedEntriesFromCache(m_GraphicsPsoCache);
	EraseUnusedEntriesFromCache(m_ComputePsoCache);

	EraseExpiredEntriesFromCache(m_InvalidGraphicsPsos);
	EraseExpiredEntriesFromCache(m_InvalidComputePsos);
}
