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

project (membuf)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "*.c" "*.C" "*.cc" "*.CC" "*.cpp" "*.CPP" "*.c++")

# 将源文件添加到统一库的源文件列表中
list(APPEND UNIFIED_SOURCES ${SOURCES})

# 将包含目录添加到统一库的包含目录列表中
list(APPEND UNIFIED_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/include
    ${TYPES_INCLUDE_PATH}
    ${MEMBUF_PRIVATE_INCLUDE_PATH}
    ${LOG_INCLUDE_PATH}
)

if(COMPILE_COVB AND (CONAN_SETTINGS_ARCH STREQUAL "x86_64"))
    add_definitions(-DENABLE_COVERAGE=1)
    list(APPEND UNIFIED_DEFINES "-DENABLE_COVERAGE=1")
endif()

# 将变量传递回父作用域
set(UNIFIED_SOURCES ${UNIFIED_SOURCES} CACHE INTERNAL "Unified sources for all libraries")
set(UNIFIED_INCLUDE_DIRS ${UNIFIED_INCLUDE_DIRS} CACHE INTERNAL "Unified include directories for all libraries")
set(UNIFIED_LINK_LIBS ${UNIFIED_LINK_LIBS} CACHE INTERNAL "Unified link libraries for all libraries")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} CACHE INTERNAL "Unified cxx flags for all libraries")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} CACHE INTERNAL "Unified c flags for all libraries")
set(UNIFIED_DEFINES ${UNIFIED_DEFINES} CACHE INTERNAL "Unified defines for all libraries")

###############################################################################
# Installation
###############################################################################
if (NOT ANDROID AND NOT WIN32)
    # 安装头文件 - 无论是否启用统一库都需要执行
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/membuf
            DESTINATION ${INCLUDE_INSTALL_DIR}
            COMPONENT headers
            FILES_MATCHING
            PATTERN "BaseType.h"
            PATTERN "buffer.h"
            PATTERN "buffer_info.h"
            PATTERN "BufferDesc.h"
            )
endif()
if (ANDROID)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/membuf
                DESTINATION ${PROJECT_BINARY_DIR}/include
                COMPONENT headers
                FILES_MATCHING
                PATTERN "BaseType.h"
                PATTERN "buffer.h"
                PATTERN "buffer_info.h"
                PATTERN "BufferDesc.h"
                )
endif()
