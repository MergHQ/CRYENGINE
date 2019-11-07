// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "CrySpatialFXAttachmentParams.h"
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
CrySpatialFXAttachmentParams::CrySpatialFXAttachmentParams(const CrySpatialFXAttachmentParams& parameters)
	: m_rtpc(parameters.m_rtpc)
	, m_nonRtpc(parameters.m_nonRtpc)
{
	m_paramChangeHandler.SetAllParamChanges();
}

//////////////////////////////////////////////////////////////////////////
AK::IAkPluginParam* CrySpatialFXAttachmentParams::Clone(AK::IAkPluginMemAlloc* pAllocator)
{
	return AK_PLUGIN_NEW(pAllocator, CrySpatialFXAttachmentParams(*this));
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CrySpatialFXAttachmentParams::Init(AK::IAkPluginMemAlloc* pAllocator, const void* pParameterBlock, AkUInt32 blockSize)
{
	if (blockSize == 0)
	{
		// Initialize default parameters here
		m_paramChangeHandler.SetAllParamChanges();
		return AK_Success;
	}

	return SetParamsBlock(pParameterBlock, blockSize);
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CrySpatialFXAttachmentParams::Term(AK::IAkPluginMemAlloc* pAllocator)
{
	AK_PLUGIN_DELETE(pAllocator, this);
	return AK_Success;
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CrySpatialFXAttachmentParams::SetParamsBlock(const void* pParameterBlock, AkUInt32 blockSize)
{
	AKRESULT result = AK_Success;

	// Read bank data here
	CHECKBANKDATASIZE(blockSize, result);
	m_paramChangeHandler.SetAllParamChanges();

	return result;
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CrySpatialFXAttachmentParams::SetParam(AkPluginParamID parameterId, const void* pValue, AkUInt32 parameterSize)
{
	AKRESULT result = AK_Success;

	// Handle parameter change here
	switch (parameterId)
	{
	case g_attachmentParamIDDummy:
		{
			m_paramChangeHandler.SetParamChange(g_attachmentParamIDDummy);
			break;
		}
	default:
		{
			result = AK_InvalidParameter;
			break;
		}
	}

	return result;
}
} // namespace Plugins
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
