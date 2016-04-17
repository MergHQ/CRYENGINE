// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:
//  Version:     v1.00
//  Created:     25/08/2015 by Jan Pinter
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef __DX12DATASTREAMER__
	#define __DX12DATASTREAMER__

	#include "DX12Base.hpp"

namespace NCryDX12 {

class CDataStreamer
{
public:
	CDataStreamer();
	~CDataStreamer();

	bool   Init(CDevice* pDevice);

	void   Flush(UINT64 fenceValue);

	UINT64 RequestUpload(CResource* destResource, UINT subresource, const D3D12_SUBRESOURCE_DATA& desc, UINT64 size);

private:
	CDevice* m_pDevice;

	struct SRequest
	{
		UINT64                 m_FenceValue;
		ID3D12Resource*        m_pTemporaryResource;

		CResource*             m_pResource;
		UINT                   m_Subresource;
		D3D12_SUBRESOURCE_DATA m_Data;
	};

	typedef std::vector<SRequest> TRequests;
	TRequests m_Requests;
};

}

#endif
