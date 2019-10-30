// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/Plugin/PluginServices/AkFXParameterChangeHandler.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
namespace Plugins
{
constexpr AkPluginParamID g_paramIDfilterFrequency = 1001;
constexpr AkUInt32 g_numParams = 1;

// These structs are intentionally left empty as they are expected by the audio engine
struct CrySpatialRTPCParams final
{
};

struct CrySpatialNonRTPCParams final
{
};

class CrySpatialFXParams final : public AK::IAkPluginParam
{
public:

	CrySpatialFXParams() = default;
	explicit CrySpatialFXParams(CrySpatialFXParams const& paramters);

	~CrySpatialFXParams() = default;

	// Create a duplicate of the parameter node instance in its current state.
	IAkPluginParam* Clone(AK::IAkPluginMemAlloc* pAllocator) override;

	// Initialize the plug-in parameter node interface.
	// Initializes the internal parameter structure to default values or with the provided parameter block if it is valid.
	AKRESULT Init(AK::IAkPluginMemAlloc* pAllocator, void const* paramterBlock, AkUInt32 blockSize) override;

	// Called by the sound engine when a parameter node is terminated.
	AKRESULT Term(AK::IAkPluginMemAlloc* pAllocator) override;

	// Set all plug-in parameters at once using a parameter block.
	AKRESULT SetParamsBlock(void const* parameterBlock, AkUInt32 blockSize) override;

	// Update a single parameter at a time and perform the necessary actions on the parameter changes.
	AKRESULT SetParam(AkPluginParamID parameterId, void const* pValue, AkUInt32 blockSize) override;

	AK::AkFXParameterChangeHandler<g_numParams> m_paramChangeHandler;

	CrySpatialRTPCParams                        m_rtpc;
	CrySpatialNonRTPCParams                     m_nonRtpc;
}

AK_ALIGN_DMA;
} // namespace Plugins
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
