// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../../DeviceManager/DeviceResourceSet.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: optimize compiled resource layout for cache friendliness
struct CDeviceResourceSet_DX11 : CDeviceResourceSet
{
	static const void* const InvalidPointer;

	struct SCompiledConstantBuffer
	{
		D3DBuffer* pBuffer;
		uint32 offset;
		uint32 size;
		uint64 code;
		EConstantBufferShaderSlot slot;
	};

	struct SCompiledUAV
	{
		ID3D11UnorderedAccessView* pUav;
		EHWShaderClass shaderType;
		int slot;
	};

	template<typename T, uint32 MaxResourceCount>
	struct SCompiledResourceRanges
	{
		SCompiledResourceRanges() { Clear(); }

		void Clear();
		void AddResource(T* pResource, const SResourceBindPoint& bindPoint);

		template<typename ExecuteForRangeFunc> void ForEach(EHWShaderClass shaderClass, ExecuteForRangeFunc executeForRange) const;
		template<typename ExecuteForRangeFunc> void ForEach(EHWShaderClass shaderClass, SCachedValue<T*>* shadowState, ExecuteForRangeFunc executeForRange) const;
		template<typename ExecuteForRangeFunc> void ForEach(EHWShaderClass shaderClass, const std::bitset<MaxResourceCount>& requestedRanges,
			SCachedValue<T*>* shadowState, ExecuteForRangeFunc executeForRange) const;

		int GetRangeCount(EHWShaderClass shaderClass) const
		{
			int result = 0;
			ForEach(shaderClass, [&result](EHWShaderClass, uint32, uint32, T* const *) { ++result; });
			return result;
		}

		std::array<T*, MaxResourceCount>                     resources;
		std::array<std::bitset<MaxResourceCount>, eHWSC_Num> ranges;
	};

	CDeviceResourceSet_DX11(CDeviceResourceSet::EFlags flags)
		: CDeviceResourceSet(flags)
	{}

	virtual bool UpdateImpl(const CDeviceResourceSetDesc& desc, CDeviceResourceSetDesc::EDirtyFlags dirtyFlags);

	void         ClearCompiledData();

	// set via reflection from shader
	// SRVs, UAVs and samplers
	SCompiledResourceRanges<ID3D11ShaderResourceView, ResourceSetMaxSrvCount>   compiledSRVs;
	SCompiledResourceRanges<ID3D11UnorderedAccessView, ResourceSetMaxUavCount>  compiledUAVs;
	SCompiledResourceRanges<ID3D11SamplerState, ResourceSetMaxSamplerCount>     compiledSamplers;

	// set directly
	std::array<std::array<SCompiledConstantBuffer, eConstantBufferShaderSlot_Count>, eHWSC_Num> compiledCBs;
	std::array<uint8, eHWSC_Num>                                                                numCompiledCBs;
};

class CDeviceResourceLayout_DX11 : public CDeviceResourceLayout
{
public:
	CDeviceResourceLayout_DX11(UsedBindSlotSet requiredResourceBindings)
		: CDeviceResourceLayout(requiredResourceBindings)
	{}
};

#include "DeviceResourceSet_D3D11.inl"
