// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct SLightVolume;

class CLightVolumeBuffer
{
public:
	CLightVolumeBuffer();

	void Create();
	void Release();
	bool HasVolumes() const    { return m_numVolumes != 0; }
	uint GetNumVolumes() const { return m_numVolumes; }

	void UpdateContent();
	void BindSRVs();

private:
	CGpuBuffer m_lightInfosBuffer;
	CGpuBuffer m_lightRangesBuffer;
	uint       m_numVolumes;
};
