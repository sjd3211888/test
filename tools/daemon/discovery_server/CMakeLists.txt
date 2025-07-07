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
cmake_minimum_required(VERSION 2.8.12)

project("DServer")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

set(lib_livbs "livbs")

#Check C++11
include(CheckCXXCompilerFlag)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    check_cxx_compiler_flag(-std=c++11 SUPPORTS_CXX11)
    if(NOT SUPPORTS_CXX11)
        message(FATAL_ERROR "Compiler doesn't support C++11")
    endif()
endif()

message(STATUS "Configuring DServer ...")
file(GLOB DISCOVERY_SERVER_SOURCES "dserver/*.cpp" "dserver/*.cxx")

set(AGENT_SOURCES
    agent/types/TopicPubSubType.cpp
    agent/PipeInstance.cpp
    agent/AgentInstance.cpp
    )

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_CONAN_PLATFORM $ENV{CONAN_PLATFORM})
    message(STATUS "CONAN_PLATFORM ENV is set to: ${CONAN_PLATFORM}")
    if(DEFINED _CONAN_PLATFORM AND NOT _CONAN_PLATFORM STREQUAL "")
        # 如果 _CONAN_PLATFORM 不为空，将其值赋给 CONAN_PLATFORM
        set(CONAN_PLATFORM "${_CONAN_PLATFORM}")
        message(STATUS "CONAN_PLATFORM is obtained from the environment variable:${CONAN_PLATFORM}")
        if(CONAN_PLATFORM STREQUAL "schu" OR CONAN_PLATFORM STREQUAL "j6" OR CONAN_PLATFORM STREQUAL "thor")
            add_definitions(-DGCC12_PLATFORM)
        endif()
    endif()
    message(STATUS "============================================DServer CONAN_PLATFORM = ${CONAN_PLATFORM}")
    if(NOT (CONAN_PLATFORM STREQUAL "s32g"
            OR CONAN_PLATFORM STREQUAL "g9h"
            OR CONAN_PLATFORM STREQUAL "schu"
            OR CONAN_PLATFORM STREQUAL "8255"
            OR CONAN_PLATFORM STREQUAL "5g"))
        # set(ASAN_OPTIONS -fsanitize=leak -fsanitize=address -lasan)
        set(ASAN_OPTIONS -fsanitize=address -fno-omit-frame-pointer)
        add_compile_options(${ASAN_OPTIONS})
        add_link_options(${ASAN_OPTIONS})
        message(STATUS "DServer open asan check")
    else()
        message(STATUS "DServer close asan check")
    endif()
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/security
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    )

add_executable(${PROJECT_NAME} main.cpp ${DISCOVERY_SERVER_SOURCES} ${AGENT_SOURCES})

target_compile_definitions(DServer PRIVATE
    $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
    $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
    $<$<BOOL:${SHM_TRANSPORT_DEFAULT}>:SHM_TRANSPORT_BUILTIN> # Enable SHM as built-in transport
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/dserver/
    ${CMAKE_CURRENT_SOURCE_DIR}/agent/
    ${CMAKE_SOURCE_DIR}/framework/vmw/include
    ${CMAKE_SOURCE_DIR}/evbs/include
    ${CMAKE_SOURCE_DIR}/rpc/include
    $<BUILD_INTERFACE:${VBS_UTILS_INCLUDE_DIR}>
    PUBLIC
    $<TARGET_PROPERTY:livbs,INTERFACE_INCLUDE_DIRECTORIES>
    )

include_directories("${ECDR_INCLUDE_PATH}")

target_link_libraries(${PROJECT_NAME} ${lib_livbs})

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tool/)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/security
    DESTINATION ${CMAKE_INSTALL_PREFIX}/tool/)

install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/Install_DServer_by_version_x64.sh
    DESTINATION ${CMAKE_INSTALL_PREFIX}/tool/)
