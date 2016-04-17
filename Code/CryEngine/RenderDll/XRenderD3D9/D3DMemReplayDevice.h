// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#pragma warning(push)
#pragma warning(disable: 28204)

#if defined(MEMREPLAY_WRAP_D3D11)
class CCryDeviceMemReplayHook : public ICryDeviceWrapperHook
{
public:
	CCryDeviceMemReplayHook();
	virtual const char* Name() const;

	/*** D3D11 Device ***/
	virtual void CreateBuffer_PostCallHook(HRESULT hr, const D3D11_BUFFER_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Buffer** ppBuffer);
	virtual void CreateTexture1D_PostCallHook(HRESULT hr, const D3D11_TEXTURE1D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture1D** ppTexture1D);
	virtual void CreateTexture2D_PostCallHook(HRESULT hr, const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D);
	virtual void CreateTexture3D_PostCallHook(HRESULT hr, const D3D11_TEXTURE3D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture3D** ppTexture3D);

	#if defined(MEMREPLAY_WRAP_XBOX_PERFORMANCE_DEVICE)
	virtual void CreatePlacementBuffer_PostCallHook(HRESULT hr, const D3D11_BUFFER_DESC* pDesc, void* pCpuVirtualAddress, ID3D11Buffer** ppBuffer);
	virtual void CreatePlacementTexture1D_PostCallHook(HRESULT hr, const D3D11_TEXTURE1D_DESC* pDesc, UINT TileModeIndex, UINT Pitch, void* pCpuVirtualAddress, ID3D11Texture1D** ppTexture1D);
	virtual void CreatePlacementTexture2D_PostCallHook(HRESULT hr, const D3D11_TEXTURE2D_DESC* pDesc, UINT TileModeIndex, UINT Pitch, void* pCpuVirtualAddress, ID3D11Texture2D** ppTexture2D);
	virtual void CreatePlacementTexture3D_PostCallHook(HRESULT hr, const D3D11_TEXTURE3D_DESC* pDesc, UINT TileModeIndex, UINT Pitch, void* pCpuVirtualAddress, ID3D11Texture3D** ppTexture3D);
	#endif // MEMREPLAY_WRAP_XBOX_PERFORMANCE_DEVICE

	#if defined(MEMREPLAY_WRAP_D3D11_CONTEXT)
	virtual void Map_PostCallHook(HRESULT hr, ID3D11Resource* pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource);
	virtual void Unmap_PreCallHook(ID3D11Resource* pResource, UINT Subresource);
	#endif // MEMREPLAY_WRAP_D3D11_CONTEXT
};

#endif // MEMREPLAY_WRAP_D3D11
