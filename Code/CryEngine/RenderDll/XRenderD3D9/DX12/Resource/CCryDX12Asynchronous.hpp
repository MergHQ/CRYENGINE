// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:
//  Version:     v1.00
//  Created:     11/02/2015 by Jan Pinter
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef __CCRYDX12ASYNCHRONOUS__
	#define __CCRYDX12ASYNCHRONOUS__

	#include "DX12/Device/CCryDX12DeviceChild.hpp"

template<typename T>
class CCryDX12Asynchronous : public CCryDX12DeviceChild<T>
{
public:
	DX12_OBJECT(CCryDX12Asynchronous, CCryDX12DeviceChild<T> );

	virtual ~CCryDX12Asynchronous()
	{

	}

	#pragma region /* ID3D11Asynchronous implementation */

	virtual UINT STDMETHODCALLTYPE GetDataSize()
	{
		return 0;
	}

	#pragma endregion

protected:
	CCryDX12Asynchronous()
		: Super(nullptr, nullptr)
	{

	}

private:

};

#endif // __CCRYDX12ASYNCHRONOUS__
