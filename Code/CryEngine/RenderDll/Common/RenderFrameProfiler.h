// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#define PP_CONCAT2(A, B) A ## B
#define PP_CONCAT(A, B)  PP_CONCAT2(A, B)

// set #if 0 here if you don't want profiling to be compiled in the code
#ifdef ENABLE_FRAME_PROFILER
	#define PROFILE_FRAME(id) CRY_PROFILE_SECTION(PROFILE_RENDERER, "Renderer:" # id)
#else
	#define PROFILE_FRAME(id)
#endif

#if defined(ENABLE_FRAME_PROFILER_LABELS)

	// macros to implement the platform differences for pushing GPU Markers
	#if CRY_RENDERER_GNM
		#define PROFILE_LABEL_GPU(_NAME)      do { GetDeviceObjectFactory().GetCoreCommandList().GetGraphicsInterface()->SetProfilerMarker (_NAME); } while (0)
		#define PROFILE_LABEL_PUSH_GPU(_NAME) do { GetDeviceObjectFactory().GetCoreCommandList().GetGraphicsInterface()->BeginProfilerEvent(_NAME); } while (0)
		#define PROFILE_LABEL_POP_GPU(_NAME)  do { GetDeviceObjectFactory().GetCoreCommandList().GetGraphicsInterface()->EndProfilerEvent  (_NAME); } while (0)
	#elif(CRY_RENDERER_VULKAN >= 10)
		#define PROFILE_LABEL_GPU(_NAME)      do { GetDeviceObjectFactory().GetCoreCommandList().GetGraphicsInterface()->SetProfilerMarker (_NAME); } while (0)
		#define PROFILE_LABEL_PUSH_GPU(_NAME) do { GetDeviceObjectFactory().GetCoreCommandList().GetGraphicsInterface()->BeginProfilerEvent(_NAME); } while (0)
		#define PROFILE_LABEL_POP_GPU(_NAME)  do { GetDeviceObjectFactory().GetCoreCommandList().GetGraphicsInterface()->EndProfilerEvent  (_NAME); } while (0)
	#elif (CRY_RENDERER_DIRECT3D >= 120)
		#define PROFILE_LABEL_GPU(     _NAME) do { GetDeviceObjectFactory().GetCoreCommandList().GetGraphicsInterface()->SetProfilerMarker (_NAME); } while (0)
		#define PROFILE_LABEL_PUSH_GPU(_NAME) do { GetDeviceObjectFactory().GetCoreCommandList().GetGraphicsInterface()->BeginProfilerEvent(_NAME); } while (0)
		#define PROFILE_LABEL_POP_GPU( _NAME) do { GetDeviceObjectFactory().GetCoreCommandList().GetGraphicsInterface()->EndProfilerEvent  (_NAME); } while (0)
	#elif (CRY_RENDERER_DIRECT3D >= 110) && (CRY_RENDERER_DIRECT3D < 120) && CRY_PLATFORM_DURANGO
		#ifdef USE_PIX_DURANGO
			#include <CryString/UnicodeFunctions.h>
			#define PROFILE_LABEL_GPU_MT(X, CTX)      do { wchar_t buf[256]; Unicode::Convert(buf, X); reinterpret_cast<ID3D11DeviceContextX*>(CTX)->PIXSetMarker(buf); } while (0)
			#define PROFILE_LABEL_PUSH_GPU_MT(X, CTX) do { wchar_t buf[256]; Unicode::Convert(buf, X); reinterpret_cast<ID3D11DeviceContextX*>(CTX)->PIXBeginEvent(buf); } while (0)
			#define PROFILE_LABEL_POP_GPU_MT(X, CTX)  do { reinterpret_cast<ID3D11DeviceContextX*>(CTX)->PIXEndEvent(); } while (0)
		#else
			#define PROFILE_LABEL_GPU_MT(X, CTX)      do { /*PIX not enabled*/ } while (0)
			#define PROFILE_LABEL_PUSH_GPU_MT(X, CTX) do { /*PIX not enabled*/ } while (0)
			#define PROFILE_LABEL_POP_GPU_MT(X, CTX)  do { /*PIX not enabled*/ } while (0)
		#endif
			#define PROFILE_LABEL_GPU(X)      PROFILE_LABEL_GPU_MT(X, gcpRendD3D->GetDeviceContext())
			#define PROFILE_LABEL_PUSH_GPU(X) PROFILE_LABEL_PUSH_GPU_MT(X, gcpRendD3D->GetDeviceContext())
			#define PROFILE_LABEL_POP_GPU(X)  PROFILE_LABEL_POP_GPU_MT(X, gcpRendD3D->GetDeviceContext())
	#elif  CRY_PLATFORM_WINDOWS
		// TODO: replace by ID3DUserDefinedAnnotation https://msdn.microsoft.com/en-us/library/hh446881.aspx
		#define PROFILE_LABEL_GPU(X)      do { wchar_t buf[256]; Unicode::Convert(buf, X); D3DPERF_SetMarker(0xffffffff, buf); } while (0)
		#define PROFILE_LABEL_PUSH_GPU(X) do { wchar_t buf[128]; Unicode::Convert(buf, X); D3DPERF_BeginEvent(0xff00ff00, buf); } while (0)
		#define PROFILE_LABEL_POP_GPU(X)  do { D3DPERF_EndEvent(); } while (0)
	#else
		#error "Profiling labels not supported on this api/platform"
	#endif

	// real push/pop marker for GPU, also add to the internal profiler and CPU Markers
	#define PROFILE_LABEL(X)      do { CRY_PROFILE_MARKER(X); PROFILE_LABEL_GPU(X); } while (0)

	#define PROFILE_LABEL_PUSH(X) \
		do { PROFILE_LABEL_PUSH_GPU(X); if (gcpRendD3D->m_pPipelineProfiler) gcpRendD3D->m_pPipelineProfiler->BeginSection(X); } while (0)
	
	#define PROFILE_LABEL_POP(X)\
		do { PROFILE_LABEL_POP_GPU(X);  if (gcpRendD3D->m_pPipelineProfiler) gcpRendD3D->m_pPipelineProfiler->EndSection(X); } while (0)

	// scope util class for GPU profiling Marker
	#define PROFILE_LABEL_SCOPE(X)                             \
	  CRY_PROFILE_SECTION(PROFILE_RENDERER, X);                \
	  class CProfileLabelScope                                 \
	  {                                                        \
	    const char* m_label;                                   \
	  public:                                                  \
	    CProfileLabelScope(const char* label) : m_label(label) \
	    { PROFILE_LABEL_PUSH(label); }                         \
	    ~CProfileLabelScope()                                  \
	    { PROFILE_LABEL_POP(m_label); }                        \
	  } PP_CONCAT(profileLabelScope, __LINE__)(X);

	// need to provide a static name for scopes with changing name for CRY_PROFILE_ macros to work
	#define PROFILE_LABEL_SCOPE_DYNAMIC(label, fixedName)         \
	  CRY_PROFILE_SECTION_ARG(PROFILE_RENDERER, fixedName, label); \
	  class CProfileLabelScope                                    \
	  {                                                           \
	    const char* m_label;                                      \
	  public:                                                     \
	    CProfileLabelScope(const char* _label)                    \
			: m_label(_label)                                     \
	    { PROFILE_LABEL_PUSH(_label); }                           \
	    ~CProfileLabelScope()                                     \
	    { PROFILE_LABEL_POP(m_label); }                           \
	  } PP_CONCAT(profileLabelScope, __LINE__)(label);

#else
// NULL implementation
	#define PROFILE_LABEL_GPU(X)
	#define PROFILE_LABEL_PUSH_GPU(X)
	#define PROFILE_LABEL_POP_GPU(X)
	#define PROFILE_LABEL(X)
	#define PROFILE_LABEL_PUSH(X)
	#define PROFILE_LABEL_POP(X)
	#define PROFILE_LABEL_SCOPE(X)
	#define PROFILE_LABEL_SCOPE_DYNAMIC(label, fixedName)

	#if CRY_PLATFORM_DURANGO
		#define PROFILE_LABEL_GPU_MT(X, CTX)
		#define PROFILE_LABEL_PUSH_GPU_MT(X, CTX)
		#define PROFILE_LABEL_POP_GPU_MT(X, CTX)
	#endif
#endif

#define FUNCTION_PROFILER_RENDER_FLAT CRY_PROFILE_FUNCTION(PROFILE_RENDERER)
