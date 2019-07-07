// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct SDeviceResourceLayoutDesc;
class CDeviceResourceSet;
struct SResourceBindPoint;
struct SResourceBinding;

class CDeviceObjectValidator
{
public:
	struct SDeviceLimits
	{
		struct SPerDraw // Per draw or dispatch
		{
			uint32 NumBindings;
			uint32 NumSamplers;
			uint32 NumConstantBuffers;
			uint32 NumShaderResources;
			uint32 NumInlineConstantBuffers;
			uint32 NumInlineShaderResources;
			uint32 NumBuffers;
			uint32 NumBufferSRVs;
			uint32 NumBufferUAVs;
			uint32 NumTextures;
			uint32 NumTextureSRVs;
			uint32 NumTextureUAVs;
			uint32 NumUAVs;
			uint32 NumSRVs;

			struct SPerStageLimits
			{
				uint32 NumSamplers;
				uint32 NumConstantBuffers; // no need to count inline and static buffers individually
				uint32 NumShaderResources;
				uint32 NumBuffers;
				uint32 NumBufferSRVs;
				uint32 NumBufferUAVs;
				uint32 NumTextures;
				uint32 NumTextureSRVs;
				uint32 NumTextureUAVs;
				uint32 NumUAVs;
				uint32 NumSRVs;
			} PerShaderStage;

		} PerDraw;

		struct SPerResourceSet
		{
			uint32 NumBuffers;
		} PerResourceSet;

	} Limits;

	static CDeviceObjectValidator Create();
	static CDeviceObjectValidator CreateForMobile();

	CDeviceObjectValidator() { ZeroStruct(Limits); }

	bool ValidateResourceLayout(const SDeviceResourceLayoutDesc& layoutDesc) const;
	bool ValidateResourceSet(const VectorMap<SResourceBindPoint, SResourceBinding>& resourceBindings) const;
};