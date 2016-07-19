// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#if defined(INCLUDE_SCALEFORM_SDK)

	#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO
// include debug libraries for scaleform
		#if defined(USE_GFX_JPG)
LINK_SYSTEM_LIBRARY("libjpeg.lib")
		#endif
		#if defined(USE_GFX_VIDEO)
LINK_SYSTEM_LIBRARY("libgfx_video.lib")
		#endif
		#if defined(USE_GFX_PNG)
LINK_SYSTEM_LIBRARY("libpng.lib")
		#endif
		#if defined(USE_GFX_IME)
LINK_SYSTEM_LIBRARY("libgfx_ime.lib")
		#endif
	#elif CRY_PLATFORM_ORBIS || CRY_PLATFORM_LINUX || CRY_PLATFORM_ANDROID || CRY_PLATFORM_APPLE
	#else
		#error Unknown ScaleForm configuration selected
	#endif

// IME dependencies
	#if CRY_PLATFORM_WINDOWS && defined(USE_GFX_IME)
LINK_SYSTEM_LIBRARY("imm32.lib")
LINK_SYSTEM_LIBRARY("oleaut32.lib")
	#endif

	#if defined(USE_GFX_VIDEO) && CRY_COMPILER_MSVC && CRY_COMPILER_VERSION >= 1900
// We need this to link the CRI library inside GfxVideo when using compiler VC14 or newer.
static auto* g_ignore = static_cast<int (*)(char*, size_t, const char*, va_list)>(&vsprintf_s);
	#endif

#endif // #ifdef INCLUDE_SCALEFORM_SDK
