// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "Renderer/SFTexture.h"
#include "Renderer/SFRenderer.h"
#include <Render/Render_TextureUtil.h>
#include <Kernel/SF_Debug.h>
#include <Render/Render_Matrix2x4.h>
#include <atomic>

namespace Scaleform {
namespace Render {

namespace
{
bool CanRescale(ETEX_Format format, ImageFormat* pTargetFormat, ResizeImageType* pResizeType)
{
	switch (format)
	{
	case eTF_R8G8B8A8:
	case eTF_B8G8R8A8:
		*pTargetFormat = Image_R8G8B8A8;
		*pResizeType = ResizeRgbaToRgba;
		return true;
	case eTF_A8:
	case eTF_R8:
		*pTargetFormat = Image_A8;
		*pResizeType = ResizeGray;
		return true;
	default:
		break;
	}
	return false;
}

bool CanGenerateMipMaps(ETEX_Format format)
{
	switch (format)
	{
	case eTF_A8:
	case eTF_R10G10B10A2:
	case eTF_R11G11B10F:
	case eTF_R16F:
	case eTF_R16S:
	case eTF_R16:
	case eTF_R16G16F:
	case eTF_R16G16S:
	case eTF_R16G16:
	case eTF_R16G16B16A16F:
	case eTF_R16G16B16A16S:
	case eTF_R16G16B16A16:
	case eTF_R32F:
	case eTF_R32G32F:
	case eTF_R32G32B32A32F:
	case eTF_R8S:
	case eTF_R8:
	case eTF_R8G8S:
	case eTF_R8G8:
	case eTF_R8G8B8A8S:
	case eTF_R8G8B8A8:
		return true;
	default:
		return false;
	}
}
}

#if defined(ENABLE_FLASH_INFO)
static std::atomic<int32> s_textureCreation;
#endif //#if defined(ENABLE_FLASH_INFO)

CSFTexture::CSFTexture(TextureManagerLocks* pLocks, const SSFFormat* pFormat, unsigned mips, const ImageSize& size, unsigned use, ImageBase* pImage)
	: Render::Texture(pLocks, size, (UByte)mips, (UInt16)use, pImage, pFormat), m_pStagingTexture(nullptr)
{
#if defined(ENABLE_FLASH_INFO)
	++s_textureCreation;
#endif  //#if defined(ENABLE_FLASH_INFO)

	TextureCount = (UByte)pFormat->GetPlaneCount();
	if (TextureCount > 1)
	{
		m_pTextures = (TexturePlane*)SF_HEAP_AUTO_ALLOC(this, sizeof(TexturePlane) * TextureCount);
	}
	else
	{
		m_pTextures = &m_texture0;
	}
	memset(m_pTextures, 0, sizeof(TexturePlane) * TextureCount);
}

CSFTexture::CSFTexture(TextureManagerLocks* pLocks, const Scaleform::String& url, ImageSize size, UByte mipLevels, const SSFFormat* pFormat)
	: Render::Texture(pLocks, size, mipLevels, 0, nullptr, pFormat), m_pStagingTexture(nullptr), m_url(url)
{
#if defined(ENABLE_FLASH_INFO)
	++s_textureCreation;
#endif  //#if defined(ENABLE_FLASH_INFO)

	TextureFlags |= TF_UserAlloc;
	m_pTextures = &m_texture0;
	m_texture0.pTexture = nullptr;
	m_texture0.planeSize = size;
}

CSFTexture::CSFTexture(TextureManagerLocks* pLocks, CTexture* pTexture, ImageSize size, ImageBase* image)
	: Render::Texture(pLocks, size, 0, 0, image, 0), m_pStagingTexture(nullptr)
{
#if defined(ENABLE_FLASH_INFO)
	++s_textureCreation;
#endif  //#if defined(ENABLE_FLASH_INFO)

	CRY_ASSERT(0);
	TextureFlags |= TF_UserAlloc;

	m_pTextures = &m_texture0;
	memset(m_pTextures, 0, sizeof(TexturePlane) * TextureCount);
	m_pTextures[0].pTexture = pTexture;
}

CSFTexture::~CSFTexture()
{
	Mutex::Locker lock(&pManagerLocks->TextureMutex);

	RemoveNode();

	if ((State == State_Valid) || (State == State_Lost))
	{
		ReleaseHWTextures();
	}

	if (m_pTextures && (m_pTextures != &m_texture0))
	{
		SF_FREE(m_pTextures);
	}
}

void CSFTexture::GetUVGenMatrix(Matrix2F* mat) const
{
	const ImageSize& size = (TextureFlags & TF_Rescale) ? ImgSize : m_pTextures[0].planeSize;
	*mat = Matrix2F::Scaling(1.0f / (float)size.Width, 1.0f / (float)size.Height);
}

bool CSFTexture::Initialize()
{
	SF_AMP_SCOPE_TIMER(static_cast<CD3D9Renderer*>(gEnv->pRenderer)->m_pRT->IsRenderThread() ? AmpServer::GetInstance().GetDisplayStats() : NULL, __FUNCTION__, Amp_Profile_Level_Medium);

	if (TextureFlags & TF_UserAlloc)
	{
		if (!m_texture0.pTexture && !m_url.IsEmpty())
		{
			State = State_Valid;
			return Render::Texture::Initialize();
		}
		return Initialize(m_texture0.pTexture);
	}

	ImageFormat sfFormat = GetImageFormat();

	if (State != State_Lost)
	{
		for (uint32 itex = 0; itex < TextureCount; itex++)
		{
			TexturePlane& tdesc = m_pTextures[itex];
			tdesc.planeSize = ImageData::GetFormatPlaneSize(sfFormat, ImgSize, itex);
		}
	}

	uint32 allocMipLevels = MipLevels;
	ETEX_Format cryFormat = GetTextureFormat()->cryFormat;

	if (Use & ImageUse_GenMipmaps)
	{
		if (CanGenerateMipMaps(cryFormat))
		{
			TextureFlags |= TF_SWMipGen;
			allocMipLevels = 31;

			for (uint32 itex = 0; itex < TextureCount; itex++)
			{
				allocMipLevels = Alg::Min(allocMipLevels, ImageSize_MipLevelCount(m_pTextures[itex].planeSize));
			}

			MipLevels = (UByte)allocMipLevels;
		}
		else
		{
			allocMipLevels = 0;
		}
		CRY_ASSERT(0 == allocMipLevels);
	}

	for (uint32 itex = 0; itex < TextureCount; itex++)
	{
		TexturePlane& tdesc = m_pTextures[itex];
		if (Use & ImageUse_RenderTarget)
		{
			tdesc.pTexture.reset();
			tdesc.pTexture.Assign_NoAddRef(CRendererResources::CreateRenderTarget(tdesc.planeSize.Width, tdesc.planeSize.Height, Clr_Transparent, cryFormat));
		}
	}

	if (pImage)
	{
		Render::Texture::Update();
	}

	State = State_Valid;
	return Render::Texture::Initialize();
}

bool CSFTexture::Initialize(CTexture* pCryTexture)
{
	if (!pCryTexture)
	{
		State = State_InitFailed;
		return false;
	}

	if (m_pTextures[0].pTexture != pCryTexture)
	{
		ReleaseHWTextures();
		m_pTextures[0].pTexture = pCryTexture;
	}

	CSFTextureManager* pManager = GetManager();
	MipLevels = pCryTexture->GetNumMips();
	pFormat = 0;

	if (pImage)
	{
		pFormat = pManager->getTextureFormat(pImage->GetFormatNoConv());
	}

	if (!pFormat)
	{
		pFormat = pManager->FindFormat(pCryTexture->GetTextureSrcFormat());
	}

	if (!pFormat)
	{
		State = State_InitFailed;
		return false;
	}

	m_pTextures[0].planeSize.SetSize(pCryTexture->GetWidth(), pCryTexture->GetHeight());

	if (ImgSize.IsEmpty())
	{
		ImgSize = m_pTextures[0].planeSize;
	}

	State = State_Valid;
	return Render::Texture::Initialize();
}

void CSFTexture::computeUpdateConvertRescaleFlags(
	bool rescale,
	bool swMipGen,
	ImageFormat inputFormat,
	ImageRescaleType& rescaleType,
	ImageFormat& rescaleBuffFromat,
	bool& convert)
{
	SF_UNUSED2(inputFormat, rescale);
	if (rescale)
	{
		convert = !CanRescale(GetTextureFormat()->cryFormat, &rescaleBuffFromat, &rescaleType);
	}

	if (GetTextureFormat()->GetScanlineCopyFn() != ImageBase::CopyScanlineDefault)
	{
		convert = CanRescale(GetTextureFormat()->cryFormat, &rescaleBuffFromat, &rescaleType);
	}

	if (swMipGen)
	{
		convert = !CanGenerateMipMaps(GetTextureFormat()->cryFormat);
	}
}

void CSFTexture::uploadImage(ImageData* pSource)
{
	CSFTextureManager* pmgr = GetManager();
	ETEX_Format format = pmgr->ToCryTextureFormat(pSource->GetFormatNoConv());
	for (uint32 itex = 0; itex < TextureCount; itex++)
	{
		TexturePlane& texDesc = m_pTextures[itex];
		for (uint32 level = 0; level < MipLevels; level++)
		{
			ImagePlane plane;
			pSource->GetPlane(level * TextureCount + itex, &plane);

			if (!texDesc.pTexture)
			{
				const unsigned char* pData = 1 == MipLevels ? plane.pData : nullptr;
				static int sTexGenID = 0;
				char name[16];
				cry_sprintf(name, "$SF_%d", ++sTexGenID);
				CTexture* pTexture = CTexture::GetOrCreate2DTexture(name, texDesc.planeSize.Width, texDesc.planeSize.Height, MipLevels, FT_DONT_STREAM, (byte*)pData, format);
				texDesc.pTexture.Assign_NoAddRef(pTexture);
				if (pData)
				{
					continue;
				}
			}

			if (!texDesc.pTexture)
			{
				continue;
			}

			CDeviceTexture* pDevTex = m_pTextures[itex].pTexture->GetDevTexture();
			if (!pDevTex)
			{
				continue;
			}

			int sizePixel(CTexture::IsBlockCompressed(format) ? CTexture::BytesPerBlock(format) : CTexture::BytesPerPixel(format));
			SResourceMemoryMapping mapping =
			{
				{ static_cast<UINT>(sizePixel), static_cast<UINT>(plane.Pitch), static_cast<UINT>(plane.DataSize), static_cast<UINT>(plane.DataSize) },
				{ 0,                            0,                              0,                                 0                                 },
				{ plane.Width,                  plane.Height,                   1,                                 1                                 } };

			GetDeviceObjectFactory().GetCoreCommandList().GetCopyInterface()->Copy(plane.pData, pDevTex, mapping);
		}
	}
}

void CSFTexture::ReleaseHWTextures(bool staging)
{
	Render::Texture::ReleaseHWTextures(staging);

	for (unsigned itex = 0; itex < TextureCount; itex++)
	{
		m_pTextures[itex].pTexture.reset();
	}

	if (staging && m_pStagingTexture)
	{
		DestroyStagingTexture(m_pStagingTexture);
		m_pStagingTexture = nullptr;
	}
}

void CSFTexture::ApplyTexture(unsigned stageIndex, const ImageFillMode& fm)
{
	if (!m_texture0.pTexture && !m_url.IsEmpty())
	{
		ICVar* pMipmapsCVar = gEnv->pConsole->GetCVar("sys_flash_mipmaps");
		uint32 mips_flag = pMipmapsCVar && pMipmapsCVar->GetIVal() ? 0 : FT_NOMIPS;
		CTexture* pTexture = static_cast<CTexture*>(gEnv->pRenderer->EF_LoadTexture(m_url, FT_DONT_STREAM | mips_flag));
		m_texture0.pTexture.Assign_NoAddRef(pTexture);

		CRY_ASSERT(
			m_texture0.pTexture &&
			m_texture0.planeSize.Width == m_texture0.pTexture->GetWidth() &&
			m_texture0.planeSize.Height == m_texture0.pTexture->GetHeight() &&
			MipLevels == m_texture0.pTexture->GetNumMips() &&
			static_cast<const SSFFormat*>(pFormat)->cryFormat == m_texture0.pTexture->GetSrcFormat());
	}

	Render::Texture::ApplyTexture(stageIndex, fm);

	CSFTextureManager* pManager = GetManager();
	SamplerStateHandle sampler = pManager->m_samplers[fm.GetIndex() % CSFTextureManager::s_samplerTypeCount];
	CTexture* views[s_maxTextureCount];

	for (uint32 view = 0; view < TextureCount; ++view)
	{
		views[view] = m_pTextures[view].pTexture;
	}

	pManager->GetRenderer()->SetSamplerState(stageIndex, TextureCount, views, sampler);
}

bool CSFTexture::Update(const UpdateDesc* updates, unsigned count, unsigned mipLevel)
{
	SF_AMP_SCOPE_RENDER_TIMER(__FUNCTION__, Amp_Profile_Level_Medium);
	if (!GetManager()->mapTexture(this, mipLevel, 1))
	{
		return false;
	}

	ImageFormat format = GetImageFormat();
	ImagePlane dplane;

	for (uint32 i = 0; i < count; ++i)
	{
		const UpdateDesc& desc = updates[i];
		ImagePlane splane(desc.SourcePlane);

		pMap->Data.GetPlane(desc.PlaneIndex, &dplane);
		dplane.pData += desc.DestRect.y1 * dplane.Pitch + desc.DestRect.x1 * GetTextureFormat()->bytesPerPixel;

		splane.SetSize(desc.DestRect.GetSize());
		dplane.SetSize(desc.DestRect.GetSize());
		ConvertImagePlane(dplane, splane, format, desc.PlaneIndex, pFormat->GetScanlineCopyFn(), 0);
	}

	GetManager()->unmapTexture(this);
	return true;
}

ImageData* CSFTexture::CreateStagingTexture()
{
	ImageData* pStagingImage = static_cast<ImageData*>(SF_MEMALIGN(sizeof(ImageData), CRY_PLATFORM_ALIGNMENT, StatRender_Text_Mem));
	new(pStagingImage) ImageData(GetImageFormat(), MipLevels, true);

	for (uint32 itex = 0; itex < TextureCount; itex++)
	{
		CSFTexture::TexturePlane& tdesc = m_pTextures[itex];
		ImagePlane plane(tdesc.planeSize, 0);

		for (uint32 level = 0; level < MipLevels; ++level)
		{
			plane.Pitch = plane.GetSize().Width;
			plane.DataSize = ImageData::GetMipLevelSize(pStagingImage->GetFormat(), plane.GetSize(), itex);
			plane.pData = static_cast<UByte*>(SF_MEMALIGN(plane.DataSize, CRY_PLATFORM_ALIGNMENT, StatRender_Text_Mem));
			pStagingImage->SetPlane(level * TextureCount + itex, plane);
			plane.SetNextMipSize();
		}
	}
	return pStagingImage;
}

void CSFTexture::DestroyStagingTexture(ImageData* pStagingImage)
{
	if (!pStagingImage)
	{
		return;
	}

	ImagePlane plane;

	for (uint32 itex = 0; itex < TextureCount; itex++)
	{
		for (uint32 level = 0; level < MipLevels; ++level)
		{
			pStagingImage->GetPlane(level * TextureCount + itex, &plane);
			CRY_ASSERT(plane.pData);
			if (plane.pData)
			{
				SF_FREE_ALIGN(plane.pData);
				plane.pData = 0;
			}
		}
	}

	pStagingImage->~ImageData();
	SF_FREE_ALIGN(pStagingImage);
	pStagingImage = nullptr;
}

bool CSFDepthStencilTexture::Initialize()
{
	pTexture.reset();
	pTexture.Assign_NoAddRef(CRendererResources::CreateDepthTarget(Size.Width, Size.Height, Clr_Transparent, eTF_D24S8));
	State = pTexture ? CSFTexture::State_Valid : CSFTexture::State_InitFailed;

	return State == CSFTexture::State_Valid;
}

bool CStagingInterface::Map(Render::Texture* pSource, unsigned mipLevel, unsigned levelCount)
{
	CRY_ASSERT(!IsMapped());
	CRY_ASSERT((mipLevel + levelCount) <= pSource->MipLevels);

	if (levelCount <= PlaneReserveSize)
	{
		Data.Initialize(pSource->GetImageFormat(), levelCount, Planes, pSource->GetPlaneCount(), true);
	}
	else if (!Data.Initialize(pSource->GetImageFormat(), levelCount, true))
	{
		return false;
	}

	if ((pSource->Use & (ImageUse_Map_Mask | ImageUse_Update | ImageUse_PartialUpdate)) == 0)
	{
		return false;
	}

	CSFTexture* pSFTexture = static_cast<CSFTexture*>(pSource);
	if (!pSFTexture->m_pStagingTexture)
	{
		pSFTexture->m_pStagingTexture = pSFTexture->CreateStagingTexture();
		if (!pSFTexture->m_pStagingTexture)
		{
			return false;
		}
	}

	uint32 textureCount = pSource->TextureCount;
	for (uint32 itex = 0; itex < textureCount; itex++)
	{
		CSFTexture::TexturePlane& tdesc = pSFTexture->m_pTextures[itex];
		ImagePlane plane(tdesc.planeSize, 0);

		for (uint32 i = 0; i < StartMipLevel; i++)
		{
			plane.SetNextMipSize();
		}

		for (uint32 level = 0; level < levelCount; level++)
		{
			pSFTexture->m_pStagingTexture->GetPlane((StartMipLevel + level) * textureCount + itex, &plane);
			Data.SetPlane(level * textureCount + itex, plane);
			plane.SetNextMipSize();
		}
	}

	pTexture = pSource;
	StartMipLevel = mipLevel;
	LevelCount = levelCount;
	pTexture->pMap = this;
	return true;
}

void CStagingInterface::Unmap(bool applyUpdate)
{
	CSFTexture* pSFTexture = static_cast<CSFTexture*>(pTexture);
	if (applyUpdate && pSFTexture->m_pStagingTexture)
	{
		pSFTexture->uploadImage(pSFTexture->m_pStagingTexture);
	}

	pTexture->pMap = 0;
	pTexture = 0;
	StartMipLevel = 0;
	LevelCount = 0;
}

CSFTextureManager::CSFTextureManager(CSFRenderer* pHal, ThreadId tid, ThreadCommandQueue* pQueue, TextureCache* pCache, Render::MemoryManager* pAllocator)
	: Render::TextureManager(tid, pQueue, pCache)
	, m_pHal(pHal)
{
	pMemoryManager = pAllocator;

	m_samplers[0] = EDefaultSamplerStates::PointWrap;
	m_samplers[1] = EDefaultSamplerStates::PointClamp;
	m_samplers[2] = CDeviceObjectFactory::GetOrCreateSamplerStateHandle(SSamplerState(FILTER_POINT, eSamplerAddressMode_Mirror, eSamplerAddressMode_Mirror, eSamplerAddressMode_Mirror, 0x0));
	m_samplers[3] = EDefaultSamplerStates::LinearWrap;
	m_samplers[4] = EDefaultSamplerStates::LinearClamp;
	m_samplers[5] = CDeviceObjectFactory::GetOrCreateSamplerStateHandle(SSamplerState(FILTER_LINEAR, eSamplerAddressMode_Mirror, eSamplerAddressMode_Mirror, eSamplerAddressMode_Mirror, 0x0));

	TextureFormats.PushBack(SF_HEAP_AUTO_NEW(this) SSFFormat(Image_R8G8B8A8, eTF_R8G8B8A8, 4, &Image::CopyScanlineDefault, &Image::CopyScanlineDefault));
	TextureFormats.PushBack(SF_HEAP_AUTO_NEW(this) SSFFormat(Image_B8G8R8A8, eTF_B8G8R8A8, 4, &Image::CopyScanlineDefault, &Image::CopyScanlineDefault));
	TextureFormats.PushBack(SF_HEAP_AUTO_NEW(this) SSFFormat(Image_R8G8B8, eTF_R8G8B8A8, 4, &Image_CopyScanline24_Extend_RGB_RGBA, &Image_CopyScanline32_Retract_RGBA_RGB));
	TextureFormats.PushBack(SF_HEAP_AUTO_NEW(this) SSFFormat(Image_B8G8R8, eTF_B8G8R8A8, 4, &Image_CopyScanline24_Extend_RGB_RGBA, &Image_CopyScanline32_Retract_RGBA_RGB));
	TextureFormats.PushBack(SF_HEAP_AUTO_NEW(this) SSFFormat(Image_A8, eTF_R8, 1, &Image::CopyScanlineDefault, &Image::CopyScanlineDefault));
	TextureFormats.PushBack(SF_HEAP_AUTO_NEW(this) SSFFormat(Image_B5G5R5A1, eTF_R8G8B8A8, 4, &Image_CopyScanline_B5G5RbA1_RGBA, NULL));
	TextureFormats.PushBack(SF_HEAP_AUTO_NEW(this) SSFFormat(Image_B5G5R5A1, eTF_R8G8B8A8, 4, &Image_CopyScanline_B5G5RbA1_BGRA, NULL));
	TextureFormats.PushBack(SF_HEAP_AUTO_NEW(this) SSFFormat(Image_G8, eTF_R8G8B8A8, 4, &Image_CopyScanline_G_RGBA, NULL));
	TextureFormats.PushBack(SF_HEAP_AUTO_NEW(this) SSFFormat(Image_G8, eTF_R8G8B8A8, 4, &Image_CopyScanline_G_RGBA, NULL));
	TextureFormats.PushBack(SF_HEAP_AUTO_NEW(this) SSFFormat(Image_DXT1, eTF_BC1, 0, &Image::CopyScanlineDefault, &Image::CopyScanlineDefault));
	TextureFormats.PushBack(SF_HEAP_AUTO_NEW(this) SSFFormat(Image_DXT3, eTF_BC2, 0, &Image::CopyScanlineDefault, &Image::CopyScanlineDefault));
	TextureFormats.PushBack(SF_HEAP_AUTO_NEW(this) SSFFormat(Image_DXT5, eTF_BC3, 0, &Image::CopyScanlineDefault, &Image::CopyScanlineDefault));
	TextureFormats.PushBack(SF_HEAP_AUTO_NEW(this) SSFFormat(Image_BC7, eTF_BC7, 0, &Image::CopyScanlineDefault, &Image::CopyScanlineDefault));
	TextureFormats.PushBack(SF_HEAP_AUTO_NEW(this) SSFFormat(Image_Y8_U2_V2, eTF_R8, 1, &Image::CopyScanlineDefault, &Image::CopyScanlineDefault));
	TextureFormats.PushBack(SF_HEAP_AUTO_NEW(this) SSFFormat(Image_Y8_U2_V2_A8, eTF_R8, 1, &Image::CopyScanlineDefault, &Image::CopyScanlineDefault));
}

CSFTextureManager::~CSFTextureManager()
{
	Mutex::Locker lock(&pLocks->TextureMutex);
	Reset();
	pLocks->pManager = 0;
}

void CSFTextureManager::Reset()
{
	Mutex::Locker lock(&pLocks->TextureMutex);
	CRY_ASSERT(TextureInitQueue.IsEmpty());

	while (!Textures.IsEmpty())
	{
		Textures.GetFirst()->LoseManager();
	}
}

void CSFTextureManager::GetTexturesStats(size_t& bytes, int32& textureCount, int32& textureCreations, std::vector<ITexture*>& renderTargets) const
{
#if defined(ENABLE_FLASH_INFO)
	Mutex::Locker lock(&pLocks->TextureMutex);

	Render::Texture* pTexture = Textures.GetFirst();
	while (!Textures.IsNull(pTexture))
	{
		if (static_cast<CSFTexture*>(pTexture)->m_texture0.pTexture)
		{
			int memRegion;
			bytes += pTexture->GetBytes(&memRegion);
			++textureCount;

			if (pTexture->Use & ImageUse_RenderTarget)
			{
				for (uint32 i = 0; i < pTexture->TextureCount; ++i)
				{
					CSFTexture* pHWtexture = static_cast<CSFTexture*>(pTexture);
					if (pHWtexture)
					{
						renderTargets.push_back(pHWtexture->m_pTextures[i].pTexture);
					}
				}
			}
		}
		pTexture = pTexture->GetNext();
	}
	textureCreations = s_textureCreation;
#endif  //#if defined(ENABLE_FLASH_INFO)
}

void CSFTextureManager::BeginScene()
{
	CTexture* views[maxTextureSlots];
	memset(views, 0, sizeof(views));
	m_pHal->SetSamplerState(0, maxTextureSlots, views, EDefaultSamplerStates::PointClamp);
}

const SSFFormat* CSFTextureManager::FindFormat(ETEX_Format format)
{
	for (unsigned i = 0; i < TextureFormats.GetSize(); i++)
	{
		if (TextureFormats[i] && static_cast<SSFFormat*>(TextureFormats[i])->cryFormat == format)
		{
			return static_cast<SSFFormat*>(TextureFormats[i]);
		}
	}
	return nullptr;
}

const SSFFormat* CSFTextureManager::FindFormat(ImageFormat format)
{
	return static_cast<const SSFFormat*>(getTextureFormat(format));
}

void CSFTextureManager::processInitTextures()
{
	if (!TextureInitQueue.IsEmpty() || !DepthStencilInitQueue.IsEmpty())
	{
		while (!TextureInitQueue.IsEmpty())
		{
			Render::Texture* pTexture = TextureInitQueue.GetFirst();
			pTexture->RemoveNode();
			if (pTexture->Initialize())
			{
				Textures.PushBack(pTexture);
			}
		}

		while (!DepthStencilInitQueue.IsEmpty())
		{
			Render::DepthStencilSurface* pdss = DepthStencilInitQueue.GetFirst();
			pdss->RemoveNode();
			pdss->Initialize();
		}
		pLocks->TextureInitWC.NotifyAll();
	}
}

Render::Texture* CSFTextureManager::CreateTexture(
	ImageFormat format,
	unsigned mipLevels,
	const ImageSize& size,
	unsigned use, ImageBase* pImage,
	Render::MemoryManager* pMemoryManager)
{
	SF_UNUSED(pMemoryManager);
	SF_AMP_SCOPE_TIMER(static_cast<CD3D9Renderer*>(gEnv->pRenderer)->m_pRT->IsRenderThread() ? AmpServer::GetInstance().GetDisplayStats() : NULL, __FUNCTION__, Amp_Profile_Level_Medium);

	const SSFFormat* pSFformat = static_cast<const SSFFormat*>(precreateTexture(format, use, pImage));
	if (!pSFformat)
	{
		return 0;
	}

	CSFTexture* pSFTexture = SF_HEAP_AUTO_NEW(this) CSFTexture(pLocks, pSFformat, mipLevels, size, use, pImage);
	return postCreateTexture(pSFTexture, use);
}

Render::Texture* CSFTextureManager::CreateTexture(CTexture* pTexture, ImageSize imgSize, ImageBase* image)
{
	if (!pTexture)
	{
		return 0;
	}

	CSFTexture* pSFTexture = SF_HEAP_AUTO_NEW(this) CSFTexture(pLocks, pTexture, imgSize, image);
	return postCreateTexture(pSFTexture, 0);
}

Render::Texture* CSFTextureManager::CreateTexture(const Scaleform::String& url, ImageSize imgSize, UByte mipLevels, const SSFFormat* pFormat)
{
	if (url.IsEmpty())
	{
		return 0;
	}

	CSFTexture* pTexture = SF_HEAP_AUTO_NEW(this) CSFTexture(pLocks, url, imgSize, mipLevels, pFormat);
	return postCreateTexture(pTexture, 0);
}

unsigned CSFTextureManager::GetTextureUseCaps(ImageFormat format)
{
	unsigned use = ImageUse_InitOnly | ImageUse_Update;
	if (!ImageData::IsFormatCompressed(format))
	{
		use |= ImageUse_PartialUpdate | ImageUse_GenMipmaps;
	}

	const Render::TextureFormat* pFormat = getTextureFormat(format);
	if (!pFormat)
	{
		return 0;
	}

	if (isScanlineCompatible(pFormat))
	{
		use |= ImageUse_MapRenderThread;
	}

	return use;
}

Render::DepthStencilSurface* CSFTextureManager::CreateDepthStencilSurface(const ImageSize& size, MemoryManager* pManager)
{
	SF_UNUSED(pManager);
	CSFDepthStencilTexture* pdss = SF_HEAP_AUTO_NEW(this) CSFDepthStencilTexture(pLocks, size);
	return postCreateDepthStencilSurface(pdss);
}

Render::DepthStencilSurface* CSFTextureManager::CreateDepthStencilSurface(CTexture* pTexture)
{
	if (!pTexture)
	{
		return 0;
	}

	CSFDepthStencilTexture* pdss = SF_HEAP_AUTO_NEW(this) CSFDepthStencilTexture(pLocks, ImageSize(pTexture->GetWidth(), pTexture->GetHeight()));
	pdss->pTexture = pTexture;
	pdss->State = CSFTexture::State_Valid;
	return pdss;
}

} // ~Render namespace
} // ~Scaleform namespace
