// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "LightVolumeBuffer.h"
#include "DriverD3D.h"
#include "Include_HLSL_CPP_Shared.h"

namespace
{

const uint maxNumVolumes = 256;
const uint maxNumLightInfos = 2048;

}

CLightVolumeBuffer::CLightVolumeBuffer()
	: m_numVolumes(0)
	, m_lightInfosBuffer(4) // two buffers per frame: regular + recursive rendering
	, m_lightRangesBuffer(4)
{
}

void CLightVolumeBuffer::Create()
{
	m_lightInfosBuffer.Create(
	  maxNumLightInfos, sizeof(SLightVolumeInfo),
	  DXGI_FORMAT_UNKNOWN, DX11BUF_BIND_SRV | DX11BUF_DYNAMIC | DX11BUF_STRUCTURED,
	  nullptr);
	m_lightRangesBuffer.Create(
	  maxNumVolumes, sizeof(SLightVolumeRange),
	  DXGI_FORMAT_UNKNOWN, DX11BUF_BIND_SRV | DX11BUF_DYNAMIC | DX11BUF_STRUCTURED,
	  nullptr);
}

void CLightVolumeBuffer::Release()
{
	m_lightInfosBuffer.Release();
	m_lightRangesBuffer.Release();
}

void CLightVolumeBuffer::UpdateContent()
{
	PROFILE_FRAME(DLightsInfo_UpdateSRV);

	CD3D9Renderer* pRenderer = gcpRendD3D;
	SRenderPipeline& RESTRICT_REFERENCE rp = gRenDev->m_RP;

	struct SLightVolume* pLightVols;
	uint32 numVols;
	gEnv->p3DEngine->GetLightVolumes(rp.m_nProcessThreadID, pLightVols, numVols);

	SLightVolumeInfo gpuStageInfos[maxNumLightInfos];
	SLightVolumeRange gpuStageRages[maxNumVolumes] = {
		{ 0 }
	};

	SLightVolumeRange currentRange;
	currentRange.begin = currentRange.end = 0;

	const uint actualNumRanges = min(numVols, maxNumVolumes);
	for (uint rangeId = 0; rangeId < actualNumRanges; ++rangeId)
	{
		const SLightVolume& cpuLightVolume = pLightVols[rangeId];
		const uint remainingSpace = maxNumLightInfos - currentRange.begin;
		const uint volumeSize = cpuLightVolume.pData.size();
		const uint actualVolumeSize = min(volumeSize, remainingSpace);
		for (uint infoId = 0; infoId < actualVolumeSize; ++infoId)
		{
			SLightVolumeInfo info;
			const SLightVolume::SLightData& cpuLightData = cpuLightVolume.pData[infoId];
			SLightVolumeInfo& gpuLightInfo = gpuStageInfos[currentRange.end++];
			gpuLightInfo.wPosition = cpuLightData.position;
			gpuLightInfo.radius = cpuLightData.radius;
			gpuLightInfo.cColor = cpuLightData.color;
			gpuLightInfo.bulbRadius = cpuLightData.buldRadius;
			gpuLightInfo.wProjectorDirection = cpuLightData.projectorDirection;
			gpuLightInfo.projectorCosAngle = cpuLightData.projectorCosAngle;
		}
		gpuStageRages[rangeId] = currentRange;
		currentRange.begin = currentRange.end;
	}

	m_lightInfosBuffer.UpdateBufferContent(gpuStageInfos, sizeof(SLightVolumeInfo) * currentRange.end);
	m_lightRangesBuffer.UpdateBufferContent(gpuStageRages, sizeof(gpuStageRages)); // TODO: Update smaller range

	m_numVolumes = actualNumRanges;
}

void CLightVolumeBuffer::BindSRVs()
{
	const uint firstSlot = 5;
	const uint numSlots = 2;
	ID3D11ShaderResourceView* views[numSlots] =
	{
		m_lightInfosBuffer.GetSRV(),
		m_lightRangesBuffer.GetSRV()
	};
	gcpRendD3D->m_DevMan.BindSRV(CDeviceManager::TYPE_VS, views, firstSlot, numSlots);
	gcpRendD3D->m_DevMan.BindSRV(CDeviceManager::TYPE_DS, views, firstSlot, numSlots);
}
