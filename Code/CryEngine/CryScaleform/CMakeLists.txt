start_sources()
sources_platform(ALL)
add_sources("CryScaleform_Uber_0.cpp"
	SOURCE_GROUP "Root"
		"Module.cpp"
		"Module.h"
		"ScaleformTypes.h"
		"ScaleformHelper.h"
		"FlashPlayerInstance.cpp"
		"FlashPlayerInstance.h"
		"SharedStates.cpp"
		"SharedStates.h"
		"SharedResources.cpp"
		"SharedResources.h"
		"GFileCryPak.cpp"
		"GFileCryPak.h"
		"GAllocatorCryMem.cpp"
		"GAllocatorCryMem.h"
		"GImeHelper.cpp"
		"GImeHelper.h"
		"GFxVideoSoundCrySound.cpp"
		"GFxVideoWrapper.cpp"
		"GFxVideoSoundCrySound.h"
		"GFxVideoWrapper.h"
)

sources_platform(ALL)
add_sources("CryScaleform_Renderer.cpp"
	SOURCE_GROUP "Renderer"
		"Renderer/SFRenderCache.cpp"
		"Renderer/SFRenderCache.h"
		"Renderer/SFRenderer.cpp"
		"Renderer/SFRenderer.h"
		"Renderer/SFMesh.cpp"
		"Renderer/SFMesh.h"
		"Renderer/SFTechnique.cpp"
		"Renderer/SFTechnique.h"
		"Renderer/SFTexture.cpp"
		"Renderer/SFTexture.h"
		"Renderer/SFConfig.h"
		"Renderer/SFShaderDescs.cpp"
		"Renderer/SFShaderDescs.h"
)

sources_platform(ALL)
add_sources("NoUberFile"
	SOURCE_GROUP "Root"
		"StdAfx.cpp"
		"StdAfx.h"
)

end_sources()

function(CryScaleformModule target)
	set(THIS_PROJECT ${target})
	project(${target})
	target_include_directories(${THIS_PROJECT} PRIVATE  ${extra_includes})
	target_include_directories(${THIS_PROJECT} PRIVATE "${SDK_DIR}")
	target_include_directories(${THIS_PROJECT} PRIVATE "${SDK_DIR}/Scaleform4/Include")
	target_include_directories(${THIS_PROJECT} PRIVATE "${SDK_DIR}/Scaleform4/Src")
	target_include_directories(${THIS_PROJECT} PRIVATE "${CRYENGINE_DIR}/Code/CryEngine/RenderDll")
	target_include_directories(${THIS_PROJECT} PRIVATE "${CRYENGINE_DIR}/Code/CryEngine/RenderDll/Common")
	target_include_directories(${THIS_PROJECT} PRIVATE "${CRYENGINE_DIR}/Code/CryEngine/RenderDll/XRenderD3D9")
	set(SCALEFORM_LIB_FOLDER "${SDK_DIR}/Scaleform4/Lib")
	set(SCALEFORM_LIBS AIR_SCU AS2_SCU AS3_SCU Expat Video GFx)
	if (WINDOWS)
		list(APPEND extra_libs winmm imm32 lz4)
		list(APPEND SCALEFORM_LIBS IME)
		set(SCALEFORM_LIB_FOLDER "${SCALEFORM_LIB_FOLDER}/win64" )
		set(THIRD_PARTY_LIBS LzmaLib libcurl expat pcre freetype libjpeg libpng zlib)
	elseif(DURANGO)
		set(SCALEFORM_LIB_FOLDER "${SCALEFORM_LIB_FOLDER}/durango" )
		set(THIRD_PARTY_LIBS LzmaLib libcurl expat pcre freetype jpeg libpng zlib)
	elseif(ORBIS)
		set(SCALEFORM_LIB_FOLDER "${SCALEFORM_LIB_FOLDER}/orbis")
		set(THIRD_PARTY_LIBS curl expat freetype jpeg LzmaLib pcre pcreposix png z)
				
		target_compile_options(${THIS_PROJECT} PRIVATE
			-Wno-parentheses
			-Wno-reorder
			-Wno-unused-value
			-Wno-unknown-pragmas
			-Wno-format
			-Wno-unused-private-field
			-Wno-multichar
			-Wno-switch
			-Wno-unused-function
			-Wno-overloaded-virtual
			-Wno-tautological-compare
			-Wno-unused-lambda-capture
			-Wno-invalid-offsetof
			-Wno-c++11-narrowing
			-Wno-empty-body
			-Wno-self-assign
			-Wno-logical-op-parentheses
			-Wno-null-conversion
		)
	endif()
	
	target_compile_definitions(${THIS_PROJECT} PRIVATE -DINCLUDE_SCALEFORM_SDK $<$<CONFIG:Debug>:SF_BUILD_DEBUG> $<$<CONFIG:Profile>:SF_BUILD_RELEASE> $<$<CONFIG:Release>:SF_BUILD_SHIPPING> ${extra_defines})

	make_library(SCALEFORM_LIBS "${SCALEFORM_LIB_FOLDER}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Profile>:Release>$<$<CONFIG:Release>:Shipping>/")
	make_library(THIRD_PARTY_LIBS "${SCALEFORM_LIB_FOLDER}/")

	target_link_libraries(${THIS_PROJECT} PRIVATE ${SCALEFORM_LIBS})
	target_link_libraries(${THIS_PROJECT} PRIVATE ${THIRD_PARTY_LIBS})
	target_link_libraries(${THIS_PROJECT} PRIVATE ${extra_libs})
endfunction()

if (OPTION_SCALEFORM4 AND (EXISTS ${SDK_DIR}/Scaleform4))
	if (WINDOWS)
		if (RENDERER_DX11)
			SET(extra_defines -DCRY_RENDERER_DIRECT3D=110)
			SET(extra_libs CryRenderD3D11 dxguid)
			SET(extra_includes)
			CryEngineModule(CryScaleformD3D11 PCH "StdAfx.cpp" SOLUTION_FOLDER "CryEngine")
			CryScaleformModule(CryScaleformD3D11)
		else()
			CryFileContainer(CryScaleformD3D11 SOLUTION_FOLDER "CryEngine")
		endif()

		if (RENDERER_DX12)
			SET(extra_defines -DCRY_RENDERER_DIRECT3D=120)
			SET(extra_libs CryRenderD3D12 d3d12 dxgi)
			SET(extra_includes)
			CryEngineModule(CryScaleformD3D12 PCH "StdAfx.cpp" SOLUTION_FOLDER "CryEngine")
			CryScaleformModule(CryScaleformD3D12)
		else()
			CryFileContainer(CryScaleformD3D12 SOLUTION_FOLDER "CryEngine")
		endif()

		if (RENDERER_VULKAN)
			SET(extra_defines -DCRY_RENDERER_VULKAN=10 -DSPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS)
			SET(extra_libs CryRenderVulkan spirv-cross)
			SET(extra_includes "${SDK_DIR}/VulkanSDK/Include")
			CryEngineModule(CryScaleformVulkan PCH "StdAfx.cpp" SOLUTION_FOLDER "CryEngine")
			CryScaleformModule(CryScaleformVulkan)
		else()
			CryFileContainer(CryScaleformVulkan SOLUTION_FOLDER "CryEngine")
		endif()

	elseif(DURANGO)
		SET(extra_defines -DCRY_RENDERER_DIRECT3D=111 -DSF_USE_STD11_THREADS)
		SET(extra_libs CryRenderD3D11 lz4)
		SET(extra_includes)
		CryEngineModule(CryScaleformD3D11 PCH "StdAfx.cpp" SOLUTION_FOLDER "CryEngine")
		CryScaleformModule(CryScaleformD3D11)
	
	elseif(ORBIS)
		SET(extra_defines -DCRY_RENDERER_GNM=40 -DSF_FLOATING_POINT_FAST -DGNM_FEATURE_VALIDATION=$<AND:$<NOT:$<CONFIG:Release>>,$<BOOL:RENDERER_GNM>>)
		SET(extra_libs CryRenderGNM SceGnf)
		SET(extra_includes)
		CryEngineModule(CryScaleformGNM PCH "StdAfx.cpp" SOLUTION_FOLDER "CryEngine")
		CryScaleformModule(CryScaleformGNM)
	endif()
endif()

