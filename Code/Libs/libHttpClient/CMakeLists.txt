# Loosely based on https://github.com/Microsoft/libHttpClient/blob/master/Utilities/CMake/CMakeLists.txt

set(OPENSSL_USE_STATIC_LIBS ${OPTION_STATIC_LINKING})

if (LINUX)
	message(WARNING "Skipping libHttpClient due to unsupported platform.")
	return()
elseif (WINDOWS)
	set(OPENSSL_ROOT_DIR "${SDK_DIR}/openssl/win_x64")
elseif (DURANGO)
	set(OPENSSL_USE_STATIC_LIBS FALSE) # no static libraries available yet
	set(OPENSSL_ROOT_DIR "${SDK_DIR}/openssl/durango")
else()
	# use system defaults
endif()

# Under MSVC, FindOpenSSL pretty much assumes WIN32. 
# So search fails on DURANGO unless we also define WIN32.
if(DURANGO AND NOT WIN32)
	set(_SHOULD_UNSET_WIN32 TRUE)
	set(WIN32 1)
endif()
find_package(OpenSSL)
if(_SHOULD_UNSET_WIN32)
	unset(WIN32)
	unset(_SHOULD_UNSET_WIN32)
endif()

if(NOT OPENSSL_FOUND)
	message(WARNING "Skipping libHttpClient due to missing OpenSSL.")
	return()
endif()

set(LIBROOT ${SDK_DIR}/libHttpClient)

start_sources()

sources_platform(ALL)
add_sources("NoUberFile"
	SOURCE_GROUP "Header Files"
		"${LIBROOT}/Include/httpClient/config.h"
		"${LIBROOT}/Include/httpClient/httpClient.h"
		"${LIBROOT}/Include/httpClient/httpProvider.h"
		"${LIBROOT}/Include/httpClient/mock.h"
		"${LIBROOT}/Include/XAsync.h"
		"${LIBROOT}/Include/XAsyncProvider.h"
		"${LIBROOT}/Include/XTaskQueue.h"
		"${LIBROOT}/Include/httpClient/trace.h"
		"${LIBROOT}/Include/httpClient/pal.h"
		"${LIBROOT}/Include/httpClient/async.h"
	SOURCE_GROUP "C++ Source\\\\Common"
		"${LIBROOT}/Source/Common/buildver.h"
		"${LIBROOT}/Source/Common/EntryList.h"
		"${LIBROOT}/Source/Common/pch.cpp"
		"${LIBROOT}/Source/Common/pch.h"
		"${LIBROOT}/Source/Common/pch_common.h"
		"${LIBROOT}/Source/Common/pal_internal.h"
		"${LIBROOT}/Source/Common/ResultMacros.h"
		"${LIBROOT}/Source/Common/uri.cpp"
		"${LIBROOT}/Source/Common/uri.h"
		"${LIBROOT}/Source/Common/utils.cpp"
		"${LIBROOT}/Source/Common/utils.h"
	SOURCE_GROUP "C++ Source\\\\Global"
		"${LIBROOT}/Source/Global/mem.cpp"
		"${LIBROOT}/Source/Global/mem.h"
		"${LIBROOT}/Source/Global/global_publics.cpp"
		"${LIBROOT}/Source/Global/global.cpp"
		"${LIBROOT}/Source/Global/global.h"
	SOURCE_GROUP "C++ Source\\\\WebSocket"
		"${LIBROOT}/Source/WebSocket/hcwebsocket.h"
		"${LIBROOT}/Source/WebSocket/hcwebsocket.cpp"
	SOURCE_GROUP "C++ Source\\\\Task"
		"${LIBROOT}/Source/Task/AsyncLib.cpp"
		"${LIBROOT}/Source/Task/AtomicVector.h"
		"${LIBROOT}/Source/Task/LocklessList.h"
		"${LIBROOT}/Source/Task/referenced_ptr.h"
		"${LIBROOT}/Source/Task/StaticArray.h"
		"${LIBROOT}/Source/Task/TaskQueue.cpp"
		"${LIBROOT}/Source/Task/TaskQueueImpl.h"
		"${LIBROOT}/Source/Task/TaskQueueP.h"
		"${LIBROOT}/Source/Task/ThreadPool.h"
		"${LIBROOT}/Source/Task/WaitTimer.h"
		"${LIBROOT}/Source/Task/XTaskQueuePriv.h"
	SOURCE_GROUP "C++ Source\\\\Mock"
		"${LIBROOT}/Source/Mock/lhc_mock.cpp"
		"${LIBROOT}/Source/Mock/lhc_mock.h"
		"${LIBROOT}/Source/Mock/mock_publics.cpp"
	SOURCE_GROUP "C++ Source\\\\HTTP"
		"${LIBROOT}/Source/HTTP/httpcall.cpp"
		"${LIBROOT}/Source/HTTP/httpcall.h"
		"${LIBROOT}/Source/HTTP/httpcall_request.cpp"
		"${LIBROOT}/Source/HTTP/httpcall_response.cpp"
	SOURCE_GROUP "C++ Source\\\\Logger"
		"${LIBROOT}/Source/Logger/trace.cpp"
		"${LIBROOT}/Source/Logger/trace_internal.h"
		"${LIBROOT}/Source/Logger/log_publics.cpp"
) # sources_platform(ALL)

if(WIN32)
add_sources("NoUberFile"
	SOURCE_GROUP "C++ Source\\\\HTTP\\\\WinHttp"
		"${LIBROOT}/Source/HTTP/WinHttp/winhttp_http_task.cpp"
		"${LIBROOT}/Source/HTTP/WinHttp/winhttp_http_task.h"
    SOURCE_GROUP "C++ Source\\\\Common\\\\Win"
		"${LIBROOT}/Source/Common/Win/utils_win.cpp"
		"${LIBROOT}/Source/Common/Win/utils_win.h"
    SOURCE_GROUP "C++ Source\\\\WebSocket\\\\Win"
		"${LIBROOT}/Source/WebSocket/Websocketpp/websocketpp_websocket.cpp"
		"${LIBROOT}/Source/WebSocket/Websocketpp/x509_cert_utilities.hpp"
		"${LIBROOT}/Source/WebSocket/WinHTTP/winhttp_websocket.cpp"
    SOURCE_GROUP "C++ Source\\\\Logger\\\\Win"
		"${LIBROOT}/Source/Logger/Win/win_logger.cpp"
    SOURCE_GROUP "C++ Source\\\\Task\\\\Win"
		"${LIBROOT}/Source/Task/ThreadPool_win32.cpp"
		"${LIBROOT}/Source/Task/WaitTimer_win32.cpp"
)
endif() # if(WIN32)

if(DURANGO)
add_sources("NoUberFile"
    SOURCE_GROUP "C++ Source\\\\HTTP\\\\XMLHttp"
		"${LIBROOT}/Source/HTTP/XMLHttp/http_buffer.cpp"
		"${LIBROOT}/Source/HTTP/XMLHttp/http_buffer.h"
		"${LIBROOT}/Source/HTTP/XMLHttp/http_request_callback.cpp"
		"${LIBROOT}/Source/HTTP/XMLHttp/http_request_callback.h"
		"${LIBROOT}/Source/HTTP/XMLHttp/http_response_stream.cpp"
		"${LIBROOT}/Source/HTTP/XMLHttp/http_response_stream.h"
		"${LIBROOT}/Source/HTTP/XMLHttp/http_request_stream.cpp"
		"${LIBROOT}/Source/HTTP/XMLHttp/http_request_stream.h"
		"${LIBROOT}/Source/HTTP/XMLHttp/xmlhttp_http_task.cpp"
		"${LIBROOT}/Source/HTTP/XMLHttp/xmlhttp_http_task.h"
    SOURCE_GROUP "C++ Source\\\\Common\\\\Win"
		"${LIBROOT}/Source/Common/Win/utils_win.cpp"
		"${LIBROOT}/Source/Common/Win/utils_win.h"
    SOURCE_GROUP "C++ Source\\\\WebSocket\\\\WinRT"
		"${LIBROOT}/Source/WebSocket/WinRT/winrt_websocket.cpp"
    SOURCE_GROUP "C++ Source\\\\Logger\\\\Win"
		"${LIBROOT}/Source/Logger/Win/win_logger.cpp"
    SOURCE_GROUP "C++ Source\\\\Task\\\\Win"
		"${LIBROOT}/Source/Task/ThreadPool_win32.cpp"
		"${LIBROOT}/Source/Task/WaitTimer_win32.cpp"
)
endif() # if(DURANGO)

if(WINDOWS_STORE)
add_sources("NoUberFile"
    SOURCE_GROUP "C++ Source\\\\HTTP\\\\XMLHttp"
		"${LIBROOT}/Source/HTTP/XMLHttp/http_buffer.cpp"
		"${LIBROOT}/Source/HTTP/XMLHttp/http_buffer.h"
		"${LIBROOT}/Source/HTTP/XMLHttp/http_request_callback.cpp"
		"${LIBROOT}/Source/HTTP/XMLHttp/http_request_callback.h"
		"${LIBROOT}/Source/HTTP/XMLHttp/http_response_stream.cpp"
		"${LIBROOT}/Source/HTTP/XMLHttp/http_response_stream.h"
		"${LIBROOT}/Source/HTTP/XMLHttp/http_request_stream.cpp"
		"${LIBROOT}/Source/HTTP/XMLHttp/http_request_stream.h"
		"${LIBROOT}/Source/HTTP/XMLHttp/xmlhttp_http_task.cpp"
		"${LIBROOT}/Source/HTTP/XMLHttp/xmlhttp_http_task.h"
    SOURCE_GROUP "C++ Source\\\\WebSocket\\\\WinRT"
		"${LIBROOT}/Source/WebSocket/WinRT/winrt_websocket.cpp"
    SOURCE_GROUP "C++ Source\\\\Common\\\\Win"
		"${LIBROOT}/Source/Common/Win/utils_win.cpp"
		"${LIBROOT}/Source/Common/Win/utils_win.h"
    SOURCE_GROUP "C++ Source\\\\Logger\\\\Win"
		"${LIBROOT}/Source/Logger/Win/win_logger.cpp"
    SOURCE_GROUP "C++ Source\\\\Task\\\\Win"
		"${LIBROOT}/Source/Task/ThreadPool_win32.cpp"
		"${LIBROOT}/Source/Task/WaitTimer_win32.cpp"
)
endif() # if(WINDOWS_STORE)

if(ANDROID)
add_sources("NoUberFile"
    SOURCE_GROUP "C++ Source\\\\HTTP\\\\Android"
		"${LIBROOT}/Source/HTTP/Android/http_android.cpp"
	   "${LIBROOT}/Source/HTTP/Android/android_http_request.cpp"
	   "${LIBROOT}/Source/HTTP/Android/android_http_request.h"
	   "${LIBROOT}/Source/HTTP/Android/android_platform_context.cpp"
	   "${LIBROOT}/Source/HTTP/Android/android_platform_context.h"
    SOURCE_GROUP "C++ Source\\\\WebSocket\\\\Android"
		"${LIBROOT}/Source/WebSocket/Websocketpp/websocketpp_websocket.cpp"
		"${LIBROOT}/Source/WebSocket/Websocketpp/x509_cert_utilities.hpp"
    SOURCE_GROUP "C++ Source\\\\Logger\\\\Android"
		"${LIBROOT}/Source/Logger/Android/android_logger.cpp"
    SOURCE_GROUP "C++ Source\\\\Task\\\\Android"
		"${LIBROOT}/Include/httpClient/async_jvm.h"
		"${LIBROOT}/Source/Task/ThreadPool_stl.cpp"
		"${LIBROOT}/Source/Task/WaitTimer_stl.cpp"
    SOURCE_GROUP "C++ Source\\\\Common\\\\Android"
		"${LIBROOT}/Source/Common/Android/utils_android.cpp"
		"${LIBROOT}/Source/Common/Android/utils_android.h"
)
endif() # if(ANDROID)

end_sources()

CryEngineStaticModule(libHttpClient SOLUTION_FOLDER "Libs")

add_dependencies(${THIS_PROJECT} OpenSSL::Crypto)
add_dependencies(${THIS_PROJECT} OpenSSL::SSL)

target_include_directories(${THIS_PROJECT} 
	PUBLIC
		${LIBROOT}/Include
	PRIVATE 
		${LIBROOT}
		${LIBROOT}/Source
		${LIBROOT}/Source/Common
		${LIBROOT}/Source/HTTP
		${LIBROOT}/Source/Logger
		${LIBROOT}/Source/WebSocket
		${LIBROOT}/Include/httpClient
		${LIBROOT}/External/asio/asio/Include
		${LIBROOT}/External/websocketpp
)

target_link_libraries(${THIS_PROJECT} PUBLIC 
	bcrypt 
	OpenSSL::Crypto
	OpenSSL::SSL
)

if(WIN32)
	target_link_libraries(${THIS_PROJECT} PUBLIC crypt32 httpapi Winhttp)
endif() # (WIN32)

target_compile_definitions(${THIS_PROJECT} PUBLIC
	-D_NO_ASYNCRTIMP -D_NO_PPLXIMP -D_NO_XSAPIIMP -DXSAPI_BUILD
)

if (MSVC)
	# Must enable RTTI 
	target_compile_options(${THIS_PROJECT} PRIVATE /GR)
	# Disable C4530 (warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc)
	target_compile_options(${THIS_PROJECT} PRIVATE /wd4530)
	# Disable two-phase name lookup as it causes error C7510 since VS2017 v15.8 (https://stackoverflow.com/questions/51864528)
	target_compile_options(${THIS_PROJECT} PRIVATE /Zc:twoPhase-)
	# Disable Zc:strictStrings (error C2440: 'initializing': cannot convert from 'const char [18]' to 'LPSTR'. note: Conversion from string literal loses const qualifier (see /Zc:strictStrings))
	target_compile_options(${THIS_PROJECT} PRIVATE /Zc:strictStrings-)
else()
	target_compile_options(${THIS_PROJECT} PRIVATE -fexceptions)
endif() # (MSVC)
