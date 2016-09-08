# Copy required additional files from 3rdparty to the binary folder

set (BinaryFileList_Win64
	Code/SDKs/XT_13_4/bin_vc14/*.dll
	Code/SDKs/Mono/bin/x64/mono-2.0.dll

	Code/SDKs/Brofiler/ProfilerCore64.dll

	Code/SDKs/audio/oculus/wwise/bin/plugins/OculusSpatializer.dll

	Code/SDKs/OpenVR/bin/win64/*.*

	Code/SDKs/OSVR/dll/*.dll

	Code/SDKs/Qt/5.6/msvc2015_64/bin/*.dll

	"Code/SDKs/Microsoft Windows SDK/10/bin/x64/d3dcompiler_47.dll"
	)

set (BinaryFileList_LINUX64
	Code/SDKs/ncurses/lib/libncursesw.so.6
	)


function(deploy_runtime_files fileexpr)
	file(GLOB FILES_TO_COPY ${fileexpr})
	foreach(FILE_PATH ${FILES_TO_COPY})
		message(STATUS "Deploy file: ${FILE_PATH}")
		get_filename_component(FILE_NAME ${FILE_PATH} NAME)

		# If another argument was passed files are deployed to the subdirectory
		if (ARGV1)
			configure_file(${FILE_PATH} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${ARGV1}/${FILE_NAME} COPYONLY)
			install(FILES ${FILE_PATH} DESTINATION bin/${ARGV1})
		else (ARGV1)
			configure_file(${FILE_PATH} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FILE_NAME} COPYONLY)
			install(FILES ${FILE_PATH} DESTINATION bin)
		endif (ARGV1)
	endforeach()
endfunction()

function(deploy_runtime_dir dir output_dir)
	file(GLOB_RECURSE FILES_TO_COPY RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/${dir} ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/*)
	
	message(STATUS "Deploy directory: ${dir}")

	foreach(FILE_NAME ${FILES_TO_COPY})
		set(FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/${FILE_NAME})

		message(${FILE_NAME})
		configure_file(${FILE_PATH} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${output_dir}/${FILE_NAME} COPYONLY)
	endforeach()

	install(DIRECTORY ${dir} DESTINATION bin/${output_dir})
endfunction()

macro(copy_binary_files_to_target)
	set( file_list_name "BinaryFileList_${BUILD_PLATFORM}" )
	get_property( BINARY_FILE_LIST VARIABLE PROPERTY ${file_list_name} )
	deploy_runtime_files("${BINARY_FILE_LIST}")

	if (ORBIS)
		deploy_runtime_files(Code/SDKs/Orbis/target/sce_module/*.prx app/sce_module)
	endif()

	if (WIN64) 
		deploy_runtime_files(Code/SDKs/Qt/5.6/msvc2015_64/plugins/platforms/*.dll platforms)
		deploy_runtime_files(Code/SDKs/Qt/5.6/msvc2015_64/plugins/imageformats/*.dll imageformats)

		# Deploy mono runtime
		#deploy_runtime_files(Code/SDKs/Mono/etc/mono/2.0/*.config mono/etc/mono/2.0)
		#deploy_runtime_files(Code/SDKs/Mono/etc/mono/4.0/*.config mono/etc/mono/4.0)
		#deploy_runtime_files(Code/SDKs/Mono/etc/mono/4.5/*.config mono/etc/mono/4.5)

		#deploy_runtime_dir(Code/SDKs/Mono/lib/mono/2.0 mono/lib/mono/2.0)
		#deploy_runtime_dir(Code/SDKs/Mono/lib/mono/3.5 mono/lib/mono/3.5)
		#deploy_runtime_dir(Code/SDKs/Mono/lib/mono/4.0 mono/lib/mono/4.0)
		#deploy_runtime_dir(Code/SDKs/Mono/lib/mono/4.5 mono/lib/mono/4.5)
		#deploy_runtime_dir(Code/SDKs/Mono/lib/mono/boo mono/lib/mono/boo)
		#deploy_runtime_dir(Code/SDKs/Mono/lib/mono/gac mono/lib/mono/gac)
		#deploy_runtime_dir(Code/SDKs/Mono/lib/mono/mono-configuration-crypto mono/lib/mono/mono-configuration-crypto)
		#deploy_runtime_dir(Code/SDKs/Mono/lib/mono/Monodoc mono/lib/mono/Monodoc)
		#deploy_runtime_dir(Code/SDKs/Mono/lib/mono/monotouch mono/lib/mono/monotouch)
		#deploy_runtime_dir(Code/SDKs/Mono/lib/mono/nuget mono/lib/mono/nuget)
		#deploy_runtime_dir(Code/SDKs/Mono/lib/mono/portable7 mono/lib/mono/portable7)
		#deploy_runtime_dir(Code/SDKs/Mono/lib/mono/portable47 mono/lib/mono/portable47)
		#deploy_runtime_dir(Code/SDKs/Mono/lib/mono/portable78 mono/lib/mono/portable78)
		#deploy_runtime_dir(Code/SDKs/Mono/lib/mono/portable259 mono/lib/mono/portable259)
		#deploy_runtime_dir("Code/SDKs/Mono/lib/mono/Reference Assemblies" "mono/lib/mono/Reference Assemblies")
		#deploy_runtime_dir(Code/SDKs/Mono/lib/mono/xbuild mono/lib/mono/xbuild)
		#deploy_runtime_dir(Code/SDKs/Mono/lib/mono/xbuild-frameworks mono/lib/mono/xbuild-frameworks)

		deploy_runtime_files("Code/SDKs/Microsoft Visual Studio Compiler/14.0/redist/x64/**/*.dll")
	endif (WIN64)
endmacro()
