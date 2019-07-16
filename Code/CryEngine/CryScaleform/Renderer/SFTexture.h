// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Renderer/SFConfig.h"
#include <Kernel/SF_List.h>
#include <Kernel/SF_Threads.h>
#include <Render/Render_Image.h>
#include <Render/Render_ThreadCommandQueue.h>
#include <Kernel/SF_HeapNew.h>
#include <Textures/Texture.h>

namespace Scaleform {
namespace Render {

class CSFRenderer;
class CSFTexture;

typedef _smart_ptr<CTexture> CTexturePtr;

class CStagingInterface : public MappedTextureBase
{
	friend CSFTexture;

public:
	CStagingInterface() : MappedTextureBase() {}

	virtual bool Map(Render::Texture* pTexture, unsigned mipLevel, unsigned levelCount);
	virtual void Unmap(bool applyUpdate = true);
};

struct SSFFormat : public TextureFormat
{
	SSFFormat(
		const ImageFormat sfFormat,
		const ETEX_Format cryFormat,
		const uint32 bytesPerPixel,
		Image::CopyScanlineFunc copyScaleformToCryFormat,
		Image::CopyScanlineFunc copyCryToScaleformFormat)
		: sfFormat(sfFormat)
		, cryFormat(cryFormat)
		, bytesPerPixel(bytesPerPixel)
		, copyScaleformToCryFormat(copyScaleformToCryFormat)
		, copyCryToScaleformFormat(copyCryToScaleformFormat)
	{}

	virtual ImageFormat             GetImageFormat() const      { return sfFormat; }
	virtual Image::CopyScanlineFunc GetScanlineCopyFn() const   { return copyScaleformToCryFormat; }
	virtual Image::CopyScanlineFunc GetScanlineUncopyFn() const { return copyCryToScaleformFormat; }

	ImageFormat             sfFormat;
	ETEX_Format             cryFormat;
	uint32                  bytesPerPixel;
	Image::CopyScanlineFunc copyScaleformToCryFormat;
	Image::CopyScanlineFunc copyCryToScaleformFormat;
};

class CSFTextureManager : public Render::TextureManager
{
	friend CSFTexture;

public:
	CSFTextureManager(CSFRenderer* pHal, ThreadId tid, ThreadCommandQueue* pQueue, TextureCache* pCache = 0, Render::MemoryManager* pAllocator = 0);
	~CSFTextureManager();

	void                                 Reset();
	CSFRenderer*                         GetRenderer() const { return m_pHal; }
	virtual void                         BeginScene();

	virtual bool                         IsDrawableImageFormat(ImageFormat format) const { return (format == Image_B8G8R8A8) || (format == Image_R8G8B8A8); }
	virtual unsigned                     GetTextureUseCaps(ImageFormat format);
	virtual Render::DepthStencilSurface* CreateDepthStencilSurface(const ImageSize& size, MemoryManager* pMemoryManager = 0);
	virtual Render::DepthStencilSurface* CreateDepthStencilSurface(CTexture* pSurface);
	virtual Render::Texture*             CreateTexture(ImageFormat format, unsigned mips, const ImageSize& size, unsigned use, ImageBase* pImage, Render::MemoryManager* pAllocator = 0);

	Render::Texture*                     CreateTexture(CTexture* pd3dtexture, ImageSize imgSize = ImageSize(0), ImageBase* image = 0);
	Render::Texture*                     CreateTexture(const Scaleform::String& url, ImageSize imgSize, UByte mipLevels, const SSFFormat* pFormat);
	void                                 GetTexturesStats(size_t& bytes, int32& textureCount, int32& textureCreations, std::vector<ITexture*>& renderTargets) const;
	const SSFFormat*                     FindFormat(ETEX_Format format);
	const SSFFormat*                     FindFormat(ImageFormat format);

	ETEX_Format                          ToCryTextureFormat(ImageFormat format)
	{
		const SSFFormat* pFormat = FindFormat(format);
		if (pFormat)
		{
			return pFormat->cryFormat;
		}
		return eTF_Unknown;
	}

protected:
	virtual MappedTextureBase& getDefaultMappedTexture() { return m_defaultMappedTexture; }
	virtual MappedTextureBase* createMappedTexture()     { return SF_HEAP_AUTO_NEW(this) CStagingInterface; }
	virtual void               processInitTextures();

public:
	enum { maxTextureSlots = 4 };

protected:
	static const uint32 s_samplerTypeCount = (Sample_Count * Wrap_Count);

	CSFRenderer*        m_pHal;
	CStagingInterface   m_defaultMappedTexture;
	SamplerStateHandle  m_samplers[s_samplerTypeCount];
};

class CSFTexture : public Render::Texture
{
public:
	static const uint32 s_maxTextureCount = 4;

	struct TexturePlane
	{
		ImageSize   planeSize;
		CTexturePtr pTexture;
	};

	TexturePlane*           m_pTextures;
	TexturePlane            m_texture0;
	ImageData*              m_pStagingTexture;
	const Scaleform::String m_url;

	CSFTexture(TextureManagerLocks* pLocks, const SSFFormat* pFormat, unsigned mips, const ImageSize& size, unsigned use, ImageBase* pImage);
	CSFTexture(TextureManagerLocks* pLocks, CTexture* pTexture, ImageSize imgSize, ImageBase* pImage);
	CSFTexture(TextureManagerLocks* pLocks, const Scaleform::String& url, ImageSize size, UByte mipLevels, const SSFFormat* pFormat);
	~CSFTexture();

	virtual ImageSize   GetTextureSize(unsigned plane = 0) const override { return m_pTextures[plane].planeSize; }
	virtual bool        IsValid() const override                          { return m_pTextures != 0; }
	virtual void        ReleaseHWTextures(bool staging = true) override;
	virtual bool        Initialize() override;
	bool                Initialize(CTexture* pTexture);

	virtual void        GetUVGenMatrix(Matrix2F* mat) const override;
	virtual bool        Update(const UpdateDesc* updates, unsigned count = 1, unsigned mipLevel = 0) override;
	virtual void        ApplyTexture(unsigned stageIndex, const ImageFillMode& fm) override;
	virtual Image*      GetImage() const override               { CRY_ASSERT(!pImage || (pImage->GetImageType() != Image::Type_ImageBase)); return (Image*)pImage; }
	virtual ImageFormat GetFormat() const override              { return GetImageFormat(); }
	const SSFFormat*    GetTextureFormat() const                { return static_cast<const SSFFormat*>(pFormat); }

	CTexture*           GetCryTexture(unsigned plane = 0) const { return m_pTextures[plane].pTexture; }
	ImageData*          CreateStagingTexture();
	void                DestroyStagingTexture(ImageData* pStagingImage);
	CSFTextureManager*  GetManager() const { return static_cast<CSFTextureManager*>(pManagerLocks->pManager); }

protected:
	virtual void computeUpdateConvertRescaleFlags(bool rescale, bool mipGen, ImageFormat inFormat, ImageRescaleType& rescaleType, ImageFormat& outFromat, bool& convert) override;
	virtual void uploadImage(ImageData* imageData) override;

	friend class CStagingInterface;
};

class CSFDepthStencilTexture : public Render::DepthStencilSurface
{
public:
	CSFDepthStencilTexture(TextureManagerLocks* pLocks, const ImageSize& size)
		: Render::DepthStencilSurface(pLocks, size)
		, pTexture(0)
	{
	}

	virtual ~CSFDepthStencilTexture() override
	{
		if (pTexture)
		{
		}
	}

	virtual bool Initialize() override;

	CTexturePtr pTexture;
};

} // ~Render namespace
} // ~Scaleform namespace
