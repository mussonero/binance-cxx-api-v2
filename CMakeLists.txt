cmake_minimum_required(VERSION 3.13.4 FATAL_ERROR)

project(binance-cxx-api-v2)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)

set(CMAKE_CXX_STANDARD 11)
add_definitions(-DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_DEBUG_POSTFIX -DNDEBUG -DOPENSSL_EXTRA -DHAVE_EX_DATA -DBUILDING_WOLFSSL -DHAVE_VISIBILITY -DWC_NO_HARDEN -DOPENSSL_ALL)

# Search path for CMake include files.   -DHAVE_EX_DATA -DBUILDING_WOLFSSL -DWC_NO_HARDEN -DHAVE_VISIBILITY -DOPENSSL_EXTRA
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external_libs/wolfssl EXCLUDE_FROM_ALL)
add_library(wolfssl::wolfssl INTERFACE IMPORTED)
set_target_properties(wolfssl::wolfssl PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
		"${CMAKE_CURRENT_SOURCE_DIR}/external_libs/wolfssl")
target_link_libraries(wolfssl::wolfssl INTERFACE wolfssl)
set(WOLFSSL_LIBRARY wolfssl::wolfssl)
set(WOLFSSL_INCLUDE_DIR
		"${CMAKE_CURRENT_BINARY_DIR}/external_libs/wolfssl")
set_property(TARGET wolfssl PROPERTY POSITION_INDEPENDENT_CODE ON)

set(WolfSSL_LIBRARY ${WOLFSSL_LIBRARY})
set(WolfSSL_INCLUDE_DIR ${WOLFSSL_INCLUDE_DIR})

option(BUILD_CURL_EXE "" OFF)
option(BUILD_SHARED_LIBS "" OFF)
option(BUILD_STATIC_LIBS "" ON)
option(BUILD_TESTING "" OFF)
option(CMAKE_USE_WOLFSSL "Enable WOLFSSL for SSL/TLS" ON)
option(CMAKE_USE_OPENSSL "Enable openSLL for SSL/TLS" OFF)
option(CURL_STATICLIB "" ON)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external_libs/curl EXCLUDE_FROM_ALL)
set_property(TARGET libcurl PROPERTY POSITION_INDEPENDENT_CODE ON)
set(CURL_INCLUDE_DIRS
		"${CMAKE_CURRENT_SOURCE_DIR}/external_libs/curl/include")
set(CURL_LIBRARIES libcurl)

if(CMAKE_USE_WOLFSSL AND NOT SSL_ENABLED)
	set(CURL_DISABLE_OPENSSL_AUTO_LOAD_CONFIG ON CACHE BOOL "Override option" FORCE)
	set(USE_OPENSSL OFF CACHE BOOL "Override option" FORCE)
	set(USE_WOLFSSL ON CACHE BOOL "Override option" FORCE)
	list(APPEND CURL_LIBS ${WOLFSSL_LIBRARY})
	include_directories(${WOLFSSL_INCLUDE_DIR})
endif()

option(LWS_WITH_WOLFSSL "Enable WOLFSSL for libwebsockets" ON)
option(LWS_STATIC_PIC "" ON)
option(LWS_HAVE_RSA_verify_pss_mgf1 "" ON)
option(LWS_WITH_NO_LOGS "" OFF)
option(LWS_WITH_SSL "" ON)
option(LWS_WITH_EXPORT_LWSTARGETS "" OFF)
option(LWS_WITHOUT_EXTENSIONS "" ON)
option(LWS_WITH_SYS_ASYNC_DNS "" ON)
option(LWS_WITH_SYS_FAULT_INJECTION "" ON)
set(LWS_WOLFSSL_INCLUDE_DIRS "${WOLFSSL_INCLUDE_DIR}")
set(LWS_WOLFSSL_LIBRARIES "${WOLFSSL_LIBRARY}")
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
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(${PROJECT_NAME} SHARED ${LIBRARY_SRC})
target_include_directories(${PROJECT_NAME} PUBLIC "${WOLFSSL_INCLUDE_DIR}")
target_include_directories(${PROJECT_NAME} PUBLIC "${LIBWEBSOCKETS_INCLUDE_DIRS}")
target_include_directories(${PROJECT_NAME} PUBLIC "${JSONCPP_INCLUDE_DIRS}")
target_include_directories(${PROJECT_NAME} PUBLIC "${CURL_INCLUDE_DIRS}")
target_link_libraries(${PROJECT_NAME} ${JSONCPP_LIBRARIES} ${CURL_LIBRARIES} ${LIBWEBSOCKETS_LIBRARIES})

add_executable(example example.cpp)
target_include_directories(example PUBLIC "${JSONCPP_INCLUDE_DIRS}")
target_link_libraries(example ${PROJECT_NAME} )

#add_executable(exampleTEST exampleTEST.cpp)
#target_include_directories(exampleTEST PUBLIC "${JSONCPP_INCLUDE_DIRS}")
#target_link_libraries(exampleTEST ${PROJECT_NAME})
#
#add_executable(httpPerformanceTest httpPerformanceTest.cpp)
#target_include_directories(httpPerformanceTest PUBLIC "${JSONCPP_INCLUDE_DIRS}")
#target_link_libraries(httpPerformanceTest ${PROJECT_NAME})
