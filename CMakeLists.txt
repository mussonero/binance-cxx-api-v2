cmake_minimum_required(VERSION 3.13.4 FATAL_ERROR)

project(binance-cxx-api-v2.1)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 2)
set(PROJECT_VERSION_PATCH 1)

set(CMAKE_CXX_STANDARD 11)
add_definitions(-DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_DEBUG_POSTFIX -DNDEBUG -DWC_NO_HARDEN -Wno-varargs -Wno-writable-strings -Wno-gnu-designator)
# Search path for CMake include files.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

## wolfssl Start ##
option(WOLFSSL_EXAMPLES "" OFF)
option(WOLFSSL_ASYNC_THREADS "" ON)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external_libs/wolfssl EXCLUDE_FROM_ALL)
add_library(wolfssl::wolfssl INTERFACE IMPORTED)
set_target_properties(wolfssl::wolfssl PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
		"${CMAKE_CURRENT_SOURCE_DIR}/external_libs/wolfssl")
target_link_libraries(wolfssl::wolfssl INTERFACE wolfssl)
set(WOLFSSL_LIBRARY wolfssl::wolfssl)
set(WOLFSSL_INCLUDE_DIR
		"${CMAKE_CURRENT_SOURCE_DIR}/external_libs/wolfssl")
set_property(TARGET wolfssl PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(wolfssl PRIVATE
		OPENSSL_EXTRA
		HAVE_EX_DATA
		BUILDING_WOLFSSL
		HAVE_VISIBILITY
		WC_NO_HARDEN
		OPENSSL_ALL
		HAVE_ECC
		WOLFSSL_ASYNC_THREADS
		)
## wolfssl End ##

## LibUV Start ##
set(BUILD_TESTING OFF CACHE BOOL "")
set(LIBUV_BUILD_TESTS OFF CACHE BOOL "")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external_libs/libuv EXCLUDE_FROM_ALL)
add_library(uv_a::uv_a INTERFACE IMPORTED)
set_target_properties(uv_a::uv_a PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
		"${CMAKE_CURRENT_SOURCE_DIR}/external_libs/libuv")
target_link_libraries(uv_a::uv_a INTERFACE uv_a)
set(LIBUV_LIBRARIES uv_a::uv_a)
set(LIBUV_INCLUDE_DIRS
		"${CMAKE_CURRENT_SOURCE_DIR}/external_libs/libuv/include")
set_property(TARGET uv_a PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(uv_a PRIVATE
		BUILD_STATIC_LIBS
		)
## LibUV End ##

## LibCurl Start ##
option(BUILD_CURL_EXE "" OFF)
option(BUILD_SHARED_LIBS "" OFF)
option(ENABLE_IPV6 "" OFF)
option(BUILD_STATIC_LIBS "" ON)
option(BUILD_TESTING "" OFF)
set(USE_WOLFSSL ON CACHE BOOL "Override option" FORCE)
option(CURL_STATICLIB "" ON)
set(WolfSSL_LIBRARY ${WOLFSSL_LIBRARY} CACHE PATH "Override option" FORCE)
set(WolfSSL_INCLUDE_DIR ${WOLFSSL_INCLUDE_DIR} CACHE PATH "Override option" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external_libs/curl EXCLUDE_FROM_ALL)
set_property(TARGET libcurl PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(libcurl PRIVATE "${WOLFSSL_INCLUDE_DIR}")
target_compile_definitions(libcurl PRIVATE
		CMAKE_USE_OPENSSL=0
		CMAKE_USE_WOLFSSL
		WC_NO_HARDEN
		CURL_DISABLE_OPENSSL_AUTO_LOAD_CONFIG
		HAVE_ECC
		OPENSSL_ALL
		OPENSSL_EXTRA
		)
set(CURL_INCLUDE_DIRS
		"${CMAKE_CURRENT_SOURCE_DIR}/external_libs/curl/include")
set(CURL_LIBRARIES libcurl)
## LibCurl End ##

## libwebsockets Start ##
set(LWS_WOLFSSL_INCLUDE_DIRS "${WOLFSSL_INCLUDE_DIR}" CACHE PATH "Override option" FORCE)
set(LWS_WOLFSSL_LIBRARIES "${WOLFSSL_LIBRARY}" CACHE PATH "Override option" FORCE)
option(LWS_WITH_SYS_ASYNC_DNS "" OFF)
option(LWS_WITH_WOLFSSL "" ON)
option(LWS_WITH_LIBUV "" ON)
option(LWS_WITH_LIBUV_INTERNAL "" ON)
option(LWS_WITH_EVLIB_PLUGINS "" OFF)
option(LWS_IPV6 "" OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external_libs/libwebsockets EXCLUDE_FROM_ALL)
set_property(TARGET websockets PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(websockets PRIVATE "${WOLFSSL_INCLUDE_DIR}")
target_compile_definitions(websockets PRIVATE
		OPENSSL_ALL
		OPENSSL_EXTRA
		HAVE_EX_DATA
		HAVE_ECC
		LWS_WITH_WOLFSSL
		LWS_WITH_SSL
		LWS_STATIC_PIC
		LWS_WITH_NO_LOGS
		)
set(LIBWEBSOCKETS_LIBRARIES websockets)
set(LIBWEBSOCKETS_INCLUDE_DIRS
		"${CMAKE_CURRENT_SOURCE_DIR}/external_libs/libwebsockets/include"
		"${CMAKE_CURRENT_BINARY_DIR}/external_libs/libwebsockets")
## libwebsockets End ##

## LibJSON Start ##
set(JSONCPP_WITH_TESTS OFF CACHE BOOL "")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external_libs/jsoncpp EXCLUDE_FROM_ALL)
set_property(TARGET jsoncpp_static PROPERTY POSITION_INDEPENDENT_CODE ON)
set(JSONCPP_INCLUDE_DIRS
		"${CMAKE_CURRENT_SOURCE_DIR}/external_libs/jsoncpp/include")
set(JSONCPP_LIBRARIES jsoncpp_static)
## LibJSON End ##

## binance-cxx-api-v2.1 Start ##
file(GLOB_RECURSE LIBRARY_SRC "src/*.cpp" "include/*.h")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(${PROJECT_NAME} SHARED ${LIBRARY_SRC})
target_include_directories(${PROJECT_NAME} PRIVATE "${WOLFSSL_INCLUDE_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${LIBUV_INCLUDE_DIRS}")
target_include_directories(${PROJECT_NAME} PRIVATE "${LIBWEBSOCKETS_INCLUDE_DIRS}")
target_include_directories(${PROJECT_NAME} PRIVATE "${JSONCPP_INCLUDE_DIRS}")
target_include_directories(${PROJECT_NAME} PRIVATE "${CURL_INCLUDE_DIRS}")
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(${PROJECT_NAME} PRIVATE
		OPENSSL_ALL
		OPENSSL_EXTRA
		HAVE_EX_DATA
		HAVE_ECC
		BUILDING_WOLFSSL
		HAVE_VISIBILITY
		WC_NO_HARDEN
		LWS_WITH_WOLFSSL
		)

target_link_libraries(${PROJECT_NAME} ${LIBUV_LIBRARIES} ${JSONCPP_LIBRARIES} ${WOLFSSL_LIBRARY} ${CURL_LIBRARIES} ${LIBWEBSOCKETS_LIBRARIES} )
## binance-cxx-api-v2.1 End ##

add_executable(example example.cpp)
target_include_directories(example PRIVATE "${JSONCPP_INCLUDE_DIRS}")
target_link_libraries(example ${PROJECT_NAME})

add_executable(exampleTEST exampleTEST.cpp)
target_include_directories(exampleTEST PRIVATE "${JSONCPP_INCLUDE_DIRS}")
target_link_libraries(exampleTEST ${PROJECT_NAME})

add_executable(httpPerformanceTest httpPerformanceTest.cpp)
target_include_directories(httpPerformanceTest PRIVATE "${JSONCPP_INCLUDE_DIRS}")
target_link_libraries(httpPerformanceTest ${PROJECT_NAME})


#https://github.com/annacrombie/plot#
## libplot Start ##
file(GLOB_RECURSE XPLOT_SRC "plot/libplot/*.c" "plot/libplot/include/*.h" "plot/libplot/include/plot/*.h" "plot/libplot/include/internal/*.h")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/plot/libplot/include)

add_library(Xplot SHARED ${XPLOT_SRC})
set_property(TARGET Xplot PROPERTY POSITION_INDEPENDENT_CODE ON)
set(XPLOT_LIBRARY Xplot)
set(XPLOT_INCLUDE_DIRS
		"${CMAKE_CURRENT_SOURCE_DIR}/plot/libplot/include")
target_compile_options(${XPLOT_LIBRARY} PRIVATE
		"-Wundef"
		"-Wmissing-include-dirs"
		"-Wold-style-definition"
		"-Wpointer-arith"
		"-Winit-self"
		"-Wstrict-prototypes"
		"-Wendif-labels"
		"-Wstrict-aliasing=2"
		"-Woverflow"
		"-Wno-missing-braces"
		"-Wno-missing-field-initializers"
		"-Wno-unused-parameter"
		)

target_link_libraries(${XPLOT_LIBRARY} m)
## libplot End ##


add_executable(exampleWatch plot/exampleWatch.cpp)
target_include_directories(exampleWatch PRIVATE  "${XPLOT_INCLUDE_DIRS}" "${JSONCPP_INCLUDE_DIRS}")
target_link_libraries(exampleWatch ${XPLOT_LIBRARY} ${PROJECT_NAME})

add_executable(exampleWatchThreads plot/exampleWatchThreads.cpp)
target_include_directories(exampleWatchThreads PRIVATE  "${XPLOT_INCLUDE_DIRS}" "${JSONCPP_INCLUDE_DIRS}")
target_link_libraries(exampleWatchThreads ${XPLOT_LIBRARY} ${PROJECT_NAME})