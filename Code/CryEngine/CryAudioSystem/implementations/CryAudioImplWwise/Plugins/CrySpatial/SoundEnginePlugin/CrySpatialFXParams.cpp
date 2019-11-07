// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "CrySpatialFXParams.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
namespace Plugins
{
//////////////////////////////////////////////////////////////////////////
CrySpatialFXParams::CrySpatialFXParams(CrySpatialFXParams const& parameters)
	: m_rtpc(parameters.m_rtpc)
	, m_nonRtpc(parameters.m_nonRtpc)
{
	m_paramChangeHandler.SetAllParamChanges();
}

//////////////////////////////////////////////////////////////////////////
AK::IAkPluginParam* CrySpatialFXParams::Clone(AK::IAkPluginMemAlloc* const pAllocator)
{
	return AK_PLUGIN_NEW(pAllocator, CrySpatialFXParams(*this));
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CrySpatialFXParams::Init(AK::IAkPluginMemAlloc* pAllocator, void const* parameterBlock, AkUInt32 blockSize)
{
	if (blockSize == 0)
	{
		// Initialize default parameters here
		m_paramChangeHandler.SetAllParamChanges();
		return AK_Success;
	}

	return SetParamsBlock(parameterBlock, blockSize);
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CrySpatialFXParams::Term(AK::IAkPluginMemAlloc* pAllocator)
{
	AK_PLUGIN_DELETE(pAllocator, this);
	return AK_Success;
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CrySpatialFXParams::SetParamsBlock(void const* parameterBlock, AkUInt32 blockSize)
{
	AKRESULT result = AK_Success;

	// Read bank data here
	CHECKBANKDATASIZE(blockSize, result);
	m_paramChangeHandler.SetAllParamChanges();

	return result;
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CrySpatialFXParams::SetParam(AkPluginParamID parameterId, void const* pValue, AkUInt32 parameterSize)
{
	//Plugin doesn't handle parameters
	return AK_Success;
}
} // namespace Plugins
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
