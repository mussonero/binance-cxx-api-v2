cmake_minimum_required(VERSION 3.13.4 FATAL_ERROR)

project(binance-cxx-api-v2)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_DEBUG_POSTFIX -DNDEBUG)

# Search path for CMake include files.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(ENABLE_TESTING "Build mbed TLS tests." OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external_libs/mbedtls EXCLUDE_FROM_ALL)

# Decorating mbedcrypto/mbedtls/mbedx509 targets to avoid the following type of errors:
# CMake Error: install(EXPORT "CURLTargets" ...) includes target "libcurl" which requires target "mbedtls"
# libcurl prefers not to add an option to disable exporting, which would actually be easier.

add_library(mbedcrypto::mbedcrypto INTERFACE IMPORTED)
set_target_properties(mbedcrypto::mbedcrypto PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
		"${CMAKE_CURRENT_SOURCE_DIR}/external_libs/mbedtls/include")
target_link_libraries(mbedcrypto::mbedcrypto INTERFACE mbedcrypto)
set(MBEDCRYPTO_LIBRARY mbedcrypto::mbedcrypto)
set_property(TARGET mbedcrypto PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(mbedtls::mbedtls INTERFACE IMPORTED)
set_target_properties(mbedtls::mbedtls PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
		"${CMAKE_CURRENT_SOURCE_DIR}/external_libs/mbedtls/include")
target_link_libraries(mbedtls::mbedtls INTERFACE mbedtls)
set(MBEDTLS_LIBRARY mbedtls::mbedtls)
set_property(TARGET mbedtls PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(mbedx509::mbedx509 INTERFACE IMPORTED)
set_target_properties(mbedx509::mbedx509 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
		"${CMAKE_CURRENT_SOURCE_DIR}/external_libs/mbedtls/include")
target_link_libraries(mbedx509::mbedx509 INTERFACE mbedtls)
set(MBEDX509_LIBRARY mbedx509::mbedx509)
set_property(TARGET mbedx509 PROPERTY POSITION_INDEPENDENT_CODE ON)

set(MBEDTLS_INCLUDE_DIRS
		"${CMAKE_CURRENT_SOURCE_DIR}/external_libs/mbedtls/include"
		"${CMAKE_CURRENT_BINARY_DIR}/external_libs/mbedtls")

set(MBEDTLS_LIBRARIES "${MBEDTLS_LIBRARY}" "${MBEDX509_LIBRARY}" "${MBEDCRYPTO_LIBRARY}")

option(BUILD_CURL_EXE "" OFF)
option(BUILD_SHARED_LIBS "" OFF)
option(BUILD_STATIC_LIBS "" ON)
option(BUILD_TESTING "" OFF)
option(CMAKE_USE_MBEDTLS "Enable mbedTLS for SSL/TLS" ON)
option(CMAKE_USE_OPENSSL "Enable openSLL for SSL/TLS" OFF)
option(CURL_STATICLIB "" ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external_libs/curl EXCLUDE_FROM_ALL)
set_property(TARGET libcurl PROPERTY POSITION_INDEPENDENT_CODE ON)
set(CURL_INCLUDE_DIRS
		"${CMAKE_CURRENT_SOURCE_DIR}/external_libs/curl/include")
set(CURL_LIBRARIES libcurl)

if(CMAKE_USE_MBEDTLS AND NOT SSL_ENABLED)
	set(SSL_ENABLED ON)
	set(USE_MBEDTLS ON)
	list(APPEND CURL_LIBS ${MBEDTLS_LIBRARIES})
	include_directories(${MBEDTLS_INCLUDE_DIR})
endif()

option(LWS_STATIC_PIC "" ON)
option(LWS_WITH_SSL "" ON)
option(LWS_WITH_MBEDTLS "Disable mbedtls for libwebsockkets" OFF)
option(LWS_WITH_NO_LOGS "" OFF)
option(LWS_WITH_EXPORT_LWSTARGETS "" OFF)
option(LWS_WITHOUT_EXTENSIONS "" ON)

#set(LWS_MBEDTLS_INCLUDE_DIRS
#		"${MBEDTLS_INCLUDE_DIRS}")
#set(LWS_MBEDTLS_LIBRARIES
#		"${MBEDTLS_LIBRARIES}")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external_libs/libwebsockets EXCLUDE_FROM_ALL)
set_property(TARGET websockets PROPERTY POSITION_INDEPENDENT_CODE ON)
set(LIBWEBSOCKETS_LIBRARIES websockets)
set(LIBWEBSOCKETS_INCLUDE_DIRS
		"${CMAKE_CURRENT_SOURCE_DIR}/external_libs/libwebsockets/include"
		"${CMAKE_CURRENT_BINARY_DIR}/external_libs/libwebsockets")

set(JSONCPP_WITH_TESTS OFF CACHE BOOL "")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external_libs/jsoncpp EXCLUDE_FROM_ALL)
set_property(TARGET jsoncpp_static PROPERTY POSITION_INDEPENDENT_CODE ON)
set(JSONCPP_INCLUDE_DIRS
		"${CMAKE_CURRENT_SOURCE_DIR}/external_libs/jsoncpp/include")
set(JSONCPP_LIBRARIES jsoncpp_static)

file(GLOB_RECURSE LIBRARY_SRC "src/*.cpp" "include/*.h")

add_library(${PROJECT_NAME} SHARED ${LIBRARY_SRC})
target_include_directories(${PROJECT_NAME} PRIVATE "${LIBWEBSOCKETS_INCLUDE_DIRS}")
target_include_directories(${PROJECT_NAME} PRIVATE "${JSONCPP_INCLUDE_DIRS}")
target_include_directories(${PROJECT_NAME} PRIVATE "${MBEDTLS_INCLUDE_DIRS}")
target_link_libraries(${PROJECT_NAME} ${LIBWEBSOCKETS_LIBRARIES} ${JSONCPP_LIBRARIES} ${CURL_LIBRARIES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(example example.cpp)
target_include_directories(example PRIVATE "${JSONCPP_INCLUDE_DIRS}")
target_link_libraries(example ${PROJECT_NAME})

add_executable(httpPerformanceTest httpPerformanceTest.cpp)
target_include_directories(httpPerformanceTest PRIVATE "${JSONCPP_INCLUDE_DIRS}")
target_link_libraries(httpPerformanceTest ${PROJECT_NAME})