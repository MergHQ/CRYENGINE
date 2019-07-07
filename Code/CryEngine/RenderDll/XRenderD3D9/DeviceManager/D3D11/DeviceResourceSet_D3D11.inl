// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

////////////////////// Optimized handling for binding ranges of resources ////////////////////////////////
template<typename T, uint32 MaxResourceCount>
void CDeviceResourceSet_DX11::SCompiledResourceRanges<T, MaxResourceCount>::Clear()
{
	resources.fill(nullptr);
	ranges.fill(0);
}

template<typename T, uint32 MaxResourceCount>
void CDeviceResourceSet_DX11::SCompiledResourceRanges<T, MaxResourceCount>::AddResource(T* pResource, const SResourceBindPoint& bindPoint)
{
	CRY_ASSERT(bindPoint.slotNumber < resources.size());
	resources[bindPoint.slotNumber] = pResource;

	// Shader stages are ordered by usage-frequency and loop exists according to usage-frequency (VS+PS fast, etc.)
	int validShaderStages = bindPoint.stages;
	for (EHWShaderClass shaderClass = eHWSC_Vertex; validShaderStages; shaderClass = EHWShaderClass(shaderClass + 1), validShaderStages >>= 1)
	{
		if (validShaderStages & 1)
			ranges[shaderClass][bindPoint.slotNumber] = true;
	}
}


template<typename T, uint32 MaxResourceCount> template<typename ExecuteForRangeFunc>
void CDeviceResourceSet_DX11::SCompiledResourceRanges<T, MaxResourceCount>::ForEach(EHWShaderClass shaderClass, ExecuteForRangeFunc executeForRange) const
{
	uint64 toBind = ranges[shaderClass].to_ullong();

	while (toBind != 0)
	{
		uint32 start = (uint32)countTrailingZeros64(toBind);
		uint64 hmask = toBind | (toBind - 1);
		uint32 end = (uint32)countTrailingZeros64(~hmask);

		executeForRange(shaderClass, start, end - start, (T**)&resources[start]);

		toBind &= (hmask + 1);
	}
}

template<typename T, uint32 MaxResourceCount> template<typename ExecuteForRangeFunc>
void CDeviceResourceSet_DX11::SCompiledResourceRanges<T, MaxResourceCount>::ForEach(EHWShaderClass shaderClass, SCachedValue<T*>* shadowState, ExecuteForRangeFunc executeForRange) const
{
	uint64 toBind = ranges[shaderClass].to_ullong();

	while (toBind != 0)
	{
		uint32 start = (uint32)countTrailingZeros64(toBind);
		uint64 hmask = toBind | (toBind - 1);
		uint32 end = (uint32)countTrailingZeros64(~hmask);

		// if any resource has changed => call executeForRange for entire range
		bool bResourceChanged = false;
		auto ppCurResource = &resources[start];

		for (int slot = start; slot < end; ++slot, ++ppCurResource)
			bResourceChanged |= shadowState[slot].Set(*ppCurResource);

		if (bResourceChanged)
		{
			executeForRange(shaderClass, start, end - start, (T**)&resources[start]);
		}

		toBind &= (hmask + 1);

	}
}

template<typename T, uint32 MaxResourceCount> template<typename ExecuteForRangeFunc>
void CDeviceResourceSet_DX11::SCompiledResourceRanges<T, MaxResourceCount>::ForEach(EHWShaderClass shaderClass, const std::bitset<MaxResourceCount>& requestedRanges, SCachedValue<T*>* shadowState,
	ExecuteForRangeFunc executeForRange) const
{
	// intersect requested set of ranges with available set of ranges
	std::bitset<MaxResourceCount> rangesToBind = (ranges[shaderClass] & requestedRanges);

	// de-duplicate resources in each range and update rangesToBind
	uint64 toBind = rangesToBind.to_ullong();

	while (toBind)
	{
		uint32 start = (uint32)countTrailingZeros64(toBind);
		uint64 hmask = toBind | (toBind - 1);
		uint32 end = (uint32)countTrailingZeros64(~hmask);

		auto ppCurResource = &resources[start];
		for (int slot = start; slot < end; ++slot, ++ppCurResource)
			rangesToBind[slot] = shadowState[slot].Set(*ppCurResource);

		toBind &= (hmask + 1);
	}

	// call executeForRange for all updated ranges
	toBind = rangesToBind.to_ullong();
	while (toBind != 0)
	{
		uint32 start = (uint32)countTrailingZeros64(toBind);
		uint64 hmask = toBind | (toBind - 1);
		uint32 end = (uint32)countTrailingZeros64(~hmask);

		executeForRange(shaderClass, start, end - start, (T**)&resources[start]);

		toBind &= (hmask + 1);
	}
}
////////////////////////////////////////// CDeviceResourceSet_DX11 ///////////////////////////////////