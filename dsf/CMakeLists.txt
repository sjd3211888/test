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

set(CMAKE_SYSTEM_NAME Linux)

project(dsfdds VERSION 1.0 LANGUAGES CXX)
option(CONAN_BUILD "Conan build option" OFF)

# Installation paths
set(BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin/ CACHE PATH "Installation directory for binaries")
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include/ CACHE PATH "Installation directory for C++ headers")
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib/ CACHE PATH "Installation directory for libraries")
set(TOOL_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/tool/ CACHE PATH "Installation directory for tool")
set(DISCOVERY_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/service/ CACHE PATH "Installation directory for discovery service")
set(EXAMPLE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/examples/ CACHE PATH "Installation directory for examples")

#log option
option(DSF_USE_ELOG "USE DSF_USE_ELOG" ON)

# statistics option
option(DSFDDS_STATISTICS "Enable DSF DDS Statistics Module" OFF)

###############################################################################
# EDDS statistics tool default setup
###############################################################################
option(EDDS_METRICS "Enable EDDS Metrics Module" ON)
if(EDDS_METRICS)
    add_definitions(-DEDDS_METRICS)
    list(APPEND UNIFIED_DEFINES "-DEDDS_METRICS")
    message("ENABLE EDDS_METRICS!")
endif()

message(STATUS "CONAN_BUILD: ${CONAN_BUILD}")
if(DSF_USE_ELOG AND CONAN_BUILD)
    message("DSF_USE_ELOG AND CONAN_BUILD")
    SET(CMAKE_C_FLAGS_DEBUG "$ENV{CFLAGS} -O3 -Wall -Werror -g -Wextra")
    SET(CMAKE_C_FLAGS_RELEASE "$ENV{CFLAGS} -O3 -Wall -Werror -Wextra")
endif ()

message(STATUS "Configuring ${PROJECT_NAME} ...")

#compiler flag setting
if ("${CMAKE_BUILD_TYPE}" STREQUAL "" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Debug"
        OR "${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG" OR "${CMAKE_BUILD_TYPE}" STREQUAL "debug")
    SET(CMAKE_BUILD_TYPE "Debug")
    SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    add_definitions(-DDEBUG)
    list(APPEND UNIFIED_DEFINES "-DDEBUG")
endif ()
message(STATUS "BuildType: ${CMAKE_BUILD_TYPE}")
#statistics setting
if (DSFDDS_STATISTICS)
    add_definitions(-DDSFDDS_STATISTICS)
    list(APPEND UNIFIED_DEFINES "-DDSFDDS_STATISTICS")
endif ()

set(LOG_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/src/common/log)
set(TYPES_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/include/)

set(MEMBUF_PRIVATE_INCLUDE_PATH
        ${PROJECT_SOURCE_DIR}/src/membuf/shared
        ${PROJECT_SOURCE_DIR}/src/membuf/core
        ${PROJECT_SOURCE_DIR}/src/membuf/utility)

set(MBUF_PRIVATE_INCLUDE_PATH
        ${PROJECT_SOURCE_DIR}/lib/message_queue/mbuf
        ${PROJECT_SOURCE_DIR}/lib/message_queue/mbuf/thirdparty/boost/include)

set(BUF_PUBLIC_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/lib/message_queue/membuf/include)
set(BUF_PRIVATE_INCLUDE_PATH ${MEMBUF_PRIVATE_INCLUDE_PATH})
set(BUF_LIB_NAME membuf)

message(STATUS "BUF_PUBLIC_INCLUDE_PATH:${BUF_PUBLIC_INCLUDE_PATH},BUF_PRIVATE_INCLUDE_PATH:${BUF_PRIVATE_INCLUDE_PATH},
BUF_LIB_NAME:${BUF_LIB_NAME}")

#external dependency
option(ENABLE_DES "Des enable option" OFF )
if (ENABLE_DES)
    add_definitions(-DENABLE_DES=1)
    list(APPEND UNIFIED_DEFINES "-DENABLE_DES=1")
    set(VBS_DES_SOURCE ${PROJECT_SOURCE_DIR}/lib/des)
    set(DES_UMD_LIB DesUmd)
endif ()

option(COMPILE_COVB "Test coverage option" OFF)
message("COMPILE_COVB Value is: ${COMPILE_COVB}")

if(COMPILE_COVB AND (CONAN_SETTINGS_ARCH STREQUAL "x86_64"))
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -fprofile-arcs -ftest-coverage -std=gnu++17 -fPIC -MD -MT -Wall -Wextra -Wpedantic -Werror -fdiagnostics-color=always")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O0 -fprofile-arcs -ftest-coverage -std=gnu++17 -fPIC -MD -MT -Wall -Wextra -Wpedantic -Werror -fdiagnostics-color=always")
    add_definitions(-DENABLE_COVERAGE=1)
    list(APPEND UNIFIED_DEFINES "-DENABLE_COVERAGE=1")
    set(ASAN_OPTIONS -fsanitize=leak -fsanitize=address -lasan)
    add_compile_options(${ASAN_OPTIONS})
    add_link_options(${ASAN_OPTIONS})
else()
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O3 -g -std=gnu++17 -fPIC -MD -MT -Wall -Wextra -Wpedantic -Werror -fdiagnostics-color=always")#-Werror=non-virtual-dtor
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -std=gnu++17 -fPIC -MD -MT -Wall -Wextra -Wpedantic -Werror -fdiagnostics-color=always")#-Werror=non-virtual-dtor
    if(CONAN_PLATFORM STREQUAL "orin")
        SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -march=armv8.2-a")
        SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=armv8.2-a")
    endif()
endif()

if (ANDROID)
    add_compile_options(-Wno-dynamic-class-memaccess -Wno-gnu-zero-variadic-macro-arguments -Wno-parentheses -Wno-unused-parameter)
    set(BUILD_EXAMPLES OFF)
    set(BUILD_TESTS OFF)
endif()

#get git commit info
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_ID
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions(-DGIT_COMMIT_ID=\"${GIT_COMMIT_ID}\")
list(APPEND UNIFIED_DEFINES "-DGIT_COMMIT_ID=\"${GIT_COMMIT_ID}\"")

#头文件获取
file(GLOB_RECURSE ALL_HEADERS
        ${PROJECT_SOURCE_DIR}/include/**/*.h
        ${PROJECT_SOURCE_DIR}/include/**/*.hpp
        ${PROJECT_SOURCE_DIR}/src/**/*.h
        ${PROJECT_SOURCE_DIR}/src/**/*.hpp
        )
#排除掉discovery目录下所有头文件
list(FILTER ALL_HEADERS EXCLUDE REGEX "${PROJECT_SOURCE_DIR}/src/discovery.*")
#MESSAGE(STATUS "ALL_HEADERS:${ALL_HEADERS}")

#获取所有源文件
file(GLOB_RECURSE ALL_SOURCES
        ${PROJECT_SOURCE_DIR}/src/**/*.cpp
        ${PROJECT_SOURCE_DIR}/src/**/*.cc
        )
#排除掉discovery目录下所有源文件
list(FILTER ALL_SOURCES EXCLUDE REGEX "${PROJECT_SOURCE_DIR}/src/discovery.*")
#MESSAGE(STATUS "ALL_SOURCES:${ALL_SOURCES}")

#排除掉libunimem目录下所有源文件
list(FILTER ALL_SOURCES EXCLUDE REGEX "${PROJECT_SOURCE_DIR}/src/unimem.*")
list(FILTER ALL_SOURCES EXCLUDE REGEX "${PROJECT_SOURCE_DIR}/src/shmmq.*")
list(FILTER ALL_SOURCES EXCLUDE REGEX "${PROJECT_SOURCE_DIR}/src/membuf.*")

if (CONAN_BUILD)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
    set(VBS_VERSION ${CONAN_PACKAGE_VERSION})
    message("VBS_VERSION is:  ${VBS_VERSION}")
    add_definitions(-DVBS_VERSION="${VBS_VERSION}")
    list(APPEND UNIFIED_DEFINES "-DVBS_VERSION=\"${VBS_VERSION}\"")
    set_property(DIRECTORY PROPERTY INCLUDE_DIRECTORIES "")

    set(GTEST_INCLUDE_PATH ${CONAN_INCLUDE_DIRS_GTEST})
    set(GTEST_LIB_PATH ${CONAN_LIB_DIRS_GTEST})
    include_directories(${GTEST_INCLUDE_PATH})
    vbs_find_thirdparty(GTest REQUIRED)
endif ()
if (ANDROID) 
    include_directories(${DDS_INCLUDE_PATH})
    set(GTEST_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/thirdparty/evbs/include)
endif()
set(${PROJECT_NAME}_source_files
        ${ALL_HEADERS}
        ${ALL_SOURCES}
        )

set(DSF_INCLUDE_PRIVATE_DIRS
    ${PROJECT_SOURCE_DIR}/lib/message_queue/include
    ${PROJECT_SOURCE_DIR}/lib/message_queue
    ${PROJECT_SOURCE_DIR}/src
    ${BUF_PRIVATE_INCLUDE_PATH}
    ${PROJECT_SOURCE_DIR}/src/discovery
    ${LOG_INCLUDE_PATH}
    ${PROJECT_SOURCE_DIR}/src/unimem/libunimem/include
    ${CMAKE_SOURCE_DIR}/utils/include
)

set(DSF_INCLUDE_PUBLIC_DIRS
    ${PROJECT_SOURCE_DIR}/include
    ${BUF_PUBLIC_INCLUDE_PATH}
)

# 将源文件添加到统一库的源文件列表中
list(APPEND UNIFIED_SOURCES ${${PROJECT_NAME}_source_files})

# 将包含目录添加到统一库的包含目录列表中
list(APPEND UNIFIED_INCLUDE_DIRS
    ${DSF_INCLUDE_PUBLIC_DIRS}
    ${DSF_INCLUDE_PRIVATE_DIRS}
)

list(APPEND UNIFIED_LINK_LIBS shmmem)

# 将变量传递回父作用域
set(UNIFIED_SOURCES ${UNIFIED_SOURCES} CACHE INTERNAL "Unified sources for all libraries")
set(UNIFIED_INCLUDE_DIRS ${UNIFIED_INCLUDE_DIRS} CACHE INTERNAL "Unified include directories for all libraries")
set(UNIFIED_LINK_LIBS ${UNIFIED_LINK_LIBS} CACHE INTERNAL "Unified link libraries for all libraries")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} CACHE INTERNAL "Unified cxx flags for all libraries")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} CACHE INTERNAL "Unified c flags for all libraries")
set(UNIFIED_DEFINES ${UNIFIED_DEFINES} CACHE INTERNAL "Unified defines for all libraries")
# 在统一库模式下添加子目录
add_subdirectory(src/unimem/)
add_subdirectory(src/shmmq)
add_subdirectory(src/membuf)

# 安装配置 - 无论是否启用统一库都需要执行
if (CONAN_BUILD)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/dsfdds
            DESTINATION ${INCLUDE_INSTALL_DIR}
            COMPONENT headers
            FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
            PATTERN "*.ipp"
            )

    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/common
            DESTINATION ${INCLUDE_INSTALL_DIR}
            COMPONENT headers
            FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
            PATTERN "*.ipp"
            )
endif()

if (ANDROID)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/dsfdds
        DESTINATION ${PROJECT_BINARY_DIR}/include
        COMPONENT headers
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*.ipp"
        )

    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/common
        DESTINATION ${PROJECT_BINARY_DIR}/include
        COMPONENT headers
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*.ipp"
        )
endif()
