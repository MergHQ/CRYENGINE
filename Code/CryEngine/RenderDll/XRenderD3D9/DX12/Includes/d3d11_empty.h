#pragma once

#define DX12_STRINGIFY(_STRING) # _STRING

#define DX12_GUID_STRING(_D0, _W0, _W1, _B0, _B1, _B2, _B3, _B4, _B5, _B6, _B7) \
	  DX12_STRINGIFY(_D0 ## - ## _W0 ## - ## _W1 ## - ## _B0 ## _B1 ## - ## _B2 ## _B3 ## _B4 ## _B5 ## _B6 ## _B7)

#define DX12_DEFINE_TYPE_GUID(_CLASS, _TYPE, _D0, _W0, _W1, _B0, _B1, _B2, _B3, _B4, _B5, _B6, _B7) \
	  _CLASS __declspec(uuid(DX12_GUID_STRING(_D0, _W0, _W1, _B0, _B1, _B2, _B3, _B4, _B5, _B6, _B7))) _TYPE;

#if defined(CRY_USE_DX12_DEVIRTUALIZED)
class CCryDX12Device;
class CCryDX12DeviceContext;
class CCryDX12BlendState;
class CCryDX12DepthStencilState;
class CCryDX12RasterizerState;
class CCryDX12SamplerState;
template<typename T> class CCryDX12Resource;
class CCryDX12Buffer;
class CCryDX12Texture1D;
class CCryDX12Texture2D;
class CCryDX12Texture3D;
template<typename T> class CCryDX12View;
class CCryDX12DepthStencilView;
class CCryDX12RenderTargetView;
class CCryDX12ShaderResourceView;
class CCryDX12UnorderedAccessView;
template<typename T> class CCryDX12Asynchronous;
class CCryDX12Query;
class CCryDX12InputLayout;


DX12_DEFINE_TYPE_GUID(class, CCryDX12Device,              DB6F6DDB, AC77, 4E88, 82, 53, 81, 9D, F9, BB, F1, 40)
DX12_DEFINE_TYPE_GUID(class, CCryDX12DeviceContext,       C0BFA96C, E089, 44FB, 8E, AF, 26, F8, 79, 61, 90, DA)
DX12_DEFINE_TYPE_GUID(class, CCryDX12BlendState,          75B68FAA, 347D, 4159, 8F, 45, A0, 64, 0F, 01, CD, 9A)
DX12_DEFINE_TYPE_GUID(class, CCryDX12DepthStencilState,   03823EFB, 8D8F, 4E1C, 9A, A2, F6, 4B, B2, CB, FD, F1)
DX12_DEFINE_TYPE_GUID(class, CCryDX12RasterizerState,     9BB4AB81, AB1A, 4D8F, B5, 06, FC, 04, 20, 0B, 6E, E7)
DX12_DEFINE_TYPE_GUID(class, CCryDX12SamplerState,        DA6FEA51, 564C, 4487, 98, 10, F0, D0, F9, B4, E3, A5)
DX12_DEFINE_TYPE_GUID(class, CCryDX12Buffer,              48570B85, D1EE, 4FCD, A2, 50, EB, 35, 07, 22, B0, 37)
DX12_DEFINE_TYPE_GUID(class, CCryDX12Texture1D,           F8FB5C27, C6B3, 4F75, A4, C8, 43, 9A, F2, EF, 56, 4C)
DX12_DEFINE_TYPE_GUID(class, CCryDX12Texture2D,           6F15AAF2, D208, 4E89, 9A, B4, 48, 95, 35, D3, 4F, 9C)
DX12_DEFINE_TYPE_GUID(class, CCryDX12Texture3D,           037E866E, F56D, 4357, A8, AF, 9D, AB, BE, 6E, 25, 0E)
DX12_DEFINE_TYPE_GUID(class, CCryDX12DepthStencilView,    9FDAC92A, 1876, 48C3, AF, AD, 25, B9, 4F, 84, A9, B6)
DX12_DEFINE_TYPE_GUID(class, CCryDX12RenderTargetView,    DFDBA067, 0B8D, 4865, 87, 5B, D7, B4, 51, 6C, C1, 64)
DX12_DEFINE_TYPE_GUID(class, CCryDX12ShaderResourceView,  B0E06FE0, 8192, 4E1A, B1, CA, 36, D7, 41, 47, 10, B2)
DX12_DEFINE_TYPE_GUID(class, CCryDX12UnorderedAccessView, 28ACF509, 7F5C, 48F6, 86, 11, F3, 16, 01, 0A, 63, 80)
DX12_DEFINE_TYPE_GUID(class, CCryDX12Query,               D6C00747, 87B7, 425E, B8, 4D, 44, D1, 08, 56, 0A, FD)
DX12_DEFINE_TYPE_GUID(class, CCryDX12InputLayout,         E4819DDC, 4CF0, 4025, BD, 26, 5D, E8, 2A, 3E, 07, B7)

class IEmptyRefCountable : public IUnknown { public: virtual ULONG STDMETHODCALLTYPE AddRef() = 0; virtual ULONG STDMETHODCALLTYPE Release() = 0; };
class IEmptyDeviceChild : public IEmptyRefCountable {};
class IEmptyDevice1 : public IEmptyRefCountable { };
class IEmptyDeviceContext1 : public IEmptyDeviceChild { };
class IEmptyState : public IEmptyDeviceChild { };
class IEmptyResource : public IEmptyDeviceChild { public: virtual void STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION *pResourceDimension) = 0; };
class IEmptyView : public IEmptyDeviceChild { /*public: virtual void GetResource(IEmptyResource **ppResource) = 0;*/ };
class IEmptyAsynchronous : public IEmptyDeviceChild { };
class IEmptyInputLayout : public IEmptyDeviceChild { };

DX12_DEFINE_TYPE_GUID(class, IEmptyDevice1,        DB6F6DDB, AC77, 4E88, 82, 53, 81, 9D, F9, BB, F1, 40)
DX12_DEFINE_TYPE_GUID(class, IEmptyDeviceContext1, C0BFA96C, E089, 44FB, 8E, AF, 26, F8, 79, 61, 90, DA)
DX12_DEFINE_TYPE_GUID(class, IEmptyState,          1841E5C8, 16B0, 489B, BC, C8, 44, CF, B0, D5, DE, AE)
DX12_DEFINE_TYPE_GUID(class, IEmptyResource,       DC8E63F3, D12B, 4952, B4, 7B, 5E, 45, 02, 6A, 86, 2D)
DX12_DEFINE_TYPE_GUID(class, IEmptyView,           839D1216, BB2E, 412B, B7, F4, A9, DB, EB, E0, 8E, D1)
DX12_DEFINE_TYPE_GUID(class, IEmptyDeviceChild,    1841E5C8, 16B0, 489B, BC, C8, 44, CF, B0, D5, DE, AE)
DX12_DEFINE_TYPE_GUID(class, IEmptyAsynchronous,   4B35D0CD, 1E15, 4258, 9C, 98, 1B, 13, 33, F6, DD, 3B)
DX12_DEFINE_TYPE_GUID(class, IEmptyInputLayout,    E4819DDC, 4CF0, 4025, BD, 26, 5D, E8, 2A, 3E, 07, B7)

#endif
