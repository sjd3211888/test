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

project(shmmq CXX C)

set(CMAKE_C_STANDARD 11)

# Installation paths
set(BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin/ CACHE PATH "Installation directory for binaries")
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include/ CACHE PATH "Installation directory for C++ headers")
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib/ CACHE PATH "Installation directory for libraries")
set(TOOL_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/tool/ CACHE PATH "Installation directory for tool")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    SET(CMAKE_BUILD_TYPE "Debug")
endif ()

option(COMPILE_COVB "Test coverage option" OFF)
message("COMPILE_COVB Value is: ${COMPILE_COVB}")
if (CONAN_BUILD)
    if(COMPILE_COVB AND (CONAN_SETTINGS_ARCH STREQUAL "x86_64"))
        SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -fprofile-arcs -ftest-coverage -std=gnu++17 -fPIC -MD -MT -Wall -Wextra -Werror -fdiagnostics-color=always")
        SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O0 -fprofile-arcs -ftest-coverage -std=gnu++17 -fPIC -MD -MT -Wall -Wextra -Werror -fdiagnostics-color=always")
        add_definitions(-DENABLE_COVERAGE=1)
        set(ASAN_OPTIONS -fsanitize=leak -fsanitize=address -lasan)
        add_compile_options(${ASAN_OPTIONS})
        add_link_options(${ASAN_OPTIONS})
    else()
        SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O3 -g -std=gnu++17 -fPIC -MD -MT -Wall -Wextra -Werror -fdiagnostics-color=always")
        SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -std=gnu++17 -fPIC -MD -MT -Wall -Wextra -Werror -fdiagnostics-color=always")
        if(CONAN_PLATFORM STREQUAL "orin")
            SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -march=armv8.2-a")
            SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=armv8.2-a")
        endif()
    endif()
endif()


SET(CMAKE_C_FLAGS_DEBUG "$ENV{CFLAGS} -O3 -Wall -Werror -Wextra")
SET(CMAKE_C_FLAGS_RELEASE "$ENV{CFLAGS} -O3 -Wall")

#获取所有源文件
file(GLOB_RECURSE all_c_files src/posix_shm.c)

file(GLOB_RECURSE all_cxx_files *.cc)

# 将源文件添加到统一库的源文件列表中
list(APPEND UNIFIED_SOURCES ${all_c_files} ${all_cxx_files})

# 将包含目录添加到统一库的包含目录列表中
list(APPEND UNIFIED_INCLUDE_DIRS
    ${LOG_INCLUDE_PATH}
    ${BUF_PRIVATE_INCLUDE_PATH}
    ${BUF_PUBLIC_INCLUDE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${TYPES_INCLUDE_PATH}
)

if (ENABLE_DES)
    list(APPEND UNIFIED_LINK_LIBS ${DES_UMD_LIB})
endif()

# 将变量传递回父作用域
set(UNIFIED_SOURCES ${UNIFIED_SOURCES} CACHE INTERNAL "Unified sources for all libraries")
set(UNIFIED_INCLUDE_DIRS ${UNIFIED_INCLUDE_DIRS} CACHE INTERNAL "Unified include directories for all libraries")
set(UNIFIED_LINK_LIBS ${UNIFIED_LINK_LIBS} CACHE INTERNAL "Unified link libraries for all libraries")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} CACHE INTERNAL "Unified cxx flags for all libraries")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} CACHE INTERNAL "Unified c flags for all libraries")
set(UNIFIED_DEFINES ${UNIFIED_DEFINES} CACHE INTERNAL "Unified defines for all libraries")
