# Copyright (c) 2025 Li Auto Inc. and its affiliates
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# ##############################################################################
# Version
# ##############################################################################
set(VBS_VERSION ${CONAN_PACKAGE_VERSION})
message("VBS_VERSION is:  ${VBS_VERSION}")
add_definitions(-DVBS_VERSION="${VBS_VERSION}")
if (NOT VBS_VERSION STREQUAL "None")
    add_definitions(-DVBS_FWK_VERSION="${VBS_VERSION}")
    list(APPEND UNIFIED_DEFINES "-DVBS_FWK_VERSION=\"${VBS_VERSION}\"")
endif()
message(STATUS "Version: ${VBS_VERSION}")

# ##############################################################################
# Project
# ##############################################################################
project(VBSFramework)
set(PROJECT_VERSION_MAJOR "${PRODUCT_MAJOR_VERSION}")
set(PROJECT_VERSION_MINOR "${PRODUCT_MINOR_VERSION}")
set(PROJECT_VERSION_PATCH "${PRODUCT_PATCH_VERSION}")
set(PROJECT_VERSION "${PRODUCT_VERSION_STR}")
set(PROJECT_NAME_LARGE "LiAuto DDS")

option(CONAN_BUILD "BUILD Framework with local CONAN" OFF)
message(STATUS "Configuring ${PROJECT_NAME_LARGE}")

# ##############################################################################
# System configuration
# ##############################################################################
set(SYSTEM_NAME "${CMAKE_SYSTEM_NAME}")
set(SYSTEM_VERSION "${CMAKE_SYSTEM_VERSION}")
set(SYSTEM_PROCESSOR "${CMAKE_SYSTEM_PROCESSOR}")

# ##############################################################################
# Build timestamp
# ##############################################################################
string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S")

set(BUILD_VBS_FWK_TOOL ON)

# ##############################################################################
# Load external projects
# ##############################################################################
if(WIN32)
    set(BUILD_VBS_FWK_TOOL OFF)
endif()

###############################################################################
# Load CMake modules
###############################################################################
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/modules)

###############################################################################
# Check libraries
###############################################################################
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
if (ANDROID)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME ON)
endif()

# ##############################################################################
# Set dependenct moudle path
# ##############################################################################
set(EVBS_ROOT_DIR ${PROJECT_SOURCE_DIR}/../evbs)
set(DSF_ROOT_DIR ${PROJECT_SOURCE_DIR}/../dsf)
set(3RD_ROOT_DIR ${PROJECT_SOURCE_DIR}/../thirdparty)
set(UTILS_ROOT_DIR ${PROJECT_SOURCE_DIR}/../utils)

# ##############################################################################
# Set C++17 as the C++ standard required to build the project
# ##############################################################################
if (ARCH STREQUAL "android-arm64")
    add_compile_definitions(ANDROID=1)
    set(BUILD_TEST OFF)
    set(BUILD_EXAMPLE OFF)
    set(lib_ertps "ertps_NDK")
    set(lib_elog "elog_NDK")
    set(lib_xmlparser "xmlparser_NDK")
    set(lib_androidlog "log")
    set(lib_dsfdds "dsfdds_NDK")
    set(lib_mbuf "membuf_NDK")
    include_directories(${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include)
endif()

set(VBS_FWK_API_VERSION_CMAKE ${VBS_VERSION})
configure_file(${PROJECT_SOURCE_DIR}/vui/include/vbs/VBSOnlineVersion.hpp ${PROJECT_SOURCE_DIR}/vui/include/vbs/VBSVersion.hpp)

# ##############################################################################
# Compile program
# ##############################################################################
set(PROJECT_IMPL_SOURCE_DIR      ${PROJECT_SOURCE_DIR}/vmw/src)
set(PROJECT_INTERFACE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/vui/src)
file(GLOB STATUS_SOURCES         ${PROJECT_INTERFACE_SOURCE_DIR}/status/*.cpp)
file(GLOB QOS_SOURCES            ${PROJECT_IMPL_SOURCE_DIR}/qos/*.cpp)
file(GLOB DYNAMIC_TYPE_SOURCES   ${PROJECT_INTERFACE_SOURCE_DIR}/types/*.cpp)

file(GLOB UTILS_SOURCES          ${PROJECT_IMPL_SOURCE_DIR}/utils/*.cpp)
file(GLOB CORE_SOURCES           ${PROJECT_IMPL_SOURCE_DIR}/core/*.cpp
                                 ${PROJECT_IMPL_SOURCE_DIR}/core/**/*.cpp
)
file(GLOB DOMAIN_SOURCES         ${PROJECT_IMPL_SOURCE_DIR}/domain/*.cpp)
file(GLOB PUB_SOURCES            ${PROJECT_IMPL_SOURCE_DIR}/pub/*.cpp
                                 ${PROJECT_IMPL_SOURCE_DIR}/pub/**/*.cpp
)
file(GLOB SUB_SOURCES            ${PROJECT_IMPL_SOURCE_DIR}/sub/*.cpp)
file(GLOB TOPIC_SOURCES          ${PROJECT_IMPL_SOURCE_DIR}/topic/*.cpp)
file(GLOB CONTEXT_SOURCES        ${PROJECT_IMPL_SOURCE_DIR}/context/*.cpp)
file(GLOB INTERFACE_SOURCES      ${PROJECT_INTERFACE_SOURCE_DIR}/*.cpp)
file(GLOB BUILTIN_SOURCES        ${PROJECT_IMPL_SOURCE_DIR}/builtin/**/*.cpp)
file(GLOB DISPATCHER_SOURCES     ${PROJECT_IMPL_SOURCE_DIR}/dispatcher/*.cpp)

if(WIN32)
    list(REMOVE_ITEM DOMAIN_SOURCES
        ${PROJECT_IMPL_SOURCE_DIR}/domain/ProxyClient.cpp
    )
    list(REMOVE_ITEM INTERFACE_SOURCES
        ${PROJECT_INTERFACE_SOURCE_DIR}/ZeroCopyDataType.cpp
    )
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    list(REMOVE_ITEM CORE_SOURCES
        ${PROJECT_IMPL_SOURCE_DIR}/core/Host.cpp)

    set(PROJECT_SOURCES
        ${CORE_SOURCES}
        ${DOMAIN_SOURCES}
        ${PUB_SOURCES}
        ${SUB_SOURCES}
        ${TOPIC_SOURCES}
        ${QOS_SOURCES}
        ${INTERFACE_SOURCES}
        ${STATUS_SOURCES}
        ${DYNAMIC_TYPE_SOURCES}
        ${BUILTIN_SOURCES}
        ${DISPATCHER_SOURCES}
        ${CONTEXT_SOURCES}
        ${UTILS_SOURCES}
    )
else()
    if(ANDROID)
        set(PROJECT_SOURCES
            ${CORE_SOURCES}
            ${DOMAIN_SOURCES}
            ${PUB_SOURCES}
            ${SUB_SOURCES}
            ${TOPIC_SOURCES}
            ${QOS_SOURCES}
            ${INTERFACE_SOURCES}
            ${STATUS_SOURCES}
            ${DYNAMIC_TYPE_SOURCES}
            ${BUILTIN_SOURCES}
            ${DISPATCHER_SOURCES}
            ${UTILS_SOURCES}
            ${CONTEXT_SOURCES}
        )
    else()
        set(PROJECT_SOURCES
            ${CORE_SOURCES}
            ${DOMAIN_SOURCES}
            ${PUB_SOURCES}
            ${SUB_SOURCES}
            ${TOPIC_SOURCES}
            ${QOS_SOURCES}
            ${INTERFACE_SOURCES}
            ${STATUS_SOURCES}
            ${DYNAMIC_TYPE_SOURCES}
            ${BUILTIN_SOURCES}
            ${DISPATCHER_SOURCES}
            ${UTILS_SOURCES}
            ${CONTEXT_SOURCES}
        )
    endif()
endif()

# 将源文件添加到统一库的源文件列表中
list(APPEND UNIFIED_SOURCES ${PROJECT_SOURCES})

# 将包含目录添加到统一库的包含目录列表中
list(APPEND UNIFIED_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/vmw/include
    ${PROJECT_SOURCE_DIR}/vmw/src
    ${PROJECT_SOURCE_DIR}/vui/include
    ${PROJECT_SOURCE_DIR}/vui/src/
    # ${DDS_INCLUDE_PATH}
    ${ECDR_INCLUDE_PATH}
    # ${DSF_INCLUDE_PATH}
    ${GTEST_INCLUDE_PATH}
    ${TINYXML2_INCLUDE_DIR}
    ${UTILS_ROOT_DIR}/include
    ${3RD_ROOT_DIR}/filewatch
    # ${THIRDPARTY_BOOST_INCLUDE_DIR}
)

# 将链接库添加到统一库的链接库列表中
list(APPEND UNIFIED_LINK_LIBS
    ${lib_ecdr}
    ${lib_ssl}
    ${lib_crypto}
)

# 将变量传递回父作用域
set(UNIFIED_SOURCES ${UNIFIED_SOURCES} CACHE INTERNAL "Unified sources for all libraries")
set(UNIFIED_INCLUDE_DIRS ${UNIFIED_INCLUDE_DIRS} CACHE INTERNAL "Unified include directories for all libraries")
set(UNIFIED_LINK_LIBS ${UNIFIED_LINK_LIBS} CACHE INTERNAL "Unified link libraries for all libraries")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} CACHE INTERNAL "Unified cxx flags for all libraries")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} CACHE INTERNAL "Unified c flags for all libraries")
set(UNIFIED_DEFINES ${UNIFIED_DEFINES} CACHE INTERNAL "Unified defines for all libraries")

if (CONAN_BUILD)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/vui/include/vbs DESTINATION ${CMAKE_INSTALL_PREFIX}/include/)
else()
    set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR})
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/vui/include/vbs/ DESTINATION ${PROJECT_BINARY_DIR}/vbs)
endif()
