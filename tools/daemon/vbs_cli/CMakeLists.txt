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

cmake_minimum_required(VERSION 3.16.3)

project(vbs_cli VERSION 1 LANGUAGES CXX)

#Check C++17
set(CMAKE_CXX_STANDARD 17)
include(CheckCXXCompilerFlag)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # 检查 GNU 或 Clang 编译器是否支持 C++17
    check_cxx_compiler_flag(-std=c++17 SUPPORTS_CXX17)
    if(NOT SUPPORTS_CXX17)
        message(FATAL_ERROR "The compiler doesn't support C++17")
    endif()
endif()
if (COMPILE_COVB)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(NOT (CONAN_PLATFORM STREQUAL "s32g"
            OR CONAN_PLATFORM STREQUAL "g9h"
            OR CONAN_PLATFORM STREQUAL "schu"
            OR CONAN_PLATFORM STREQUAL "8255"
            OR CONAN_PLATFORM STREQUAL "5g"))
        if(CONAN_PLATFORM STREQUAL "schu" OR CONAN_PLATFORM STREQUAL "j6" OR CONAN_PLATFORM STREQUAL "thor" OR CONAN_PLATFORM STREQUAL "gcc13.3")
            set(ASAN_OPTIONS -fsanitize=address -fno-omit-frame-pointer -Wno-maybe-uninitialized)
        else()
            set(ASAN_OPTIONS -fsanitize=address -fno-omit-frame-pointer)
        endif()
        add_compile_options(${ASAN_OPTIONS})
        add_link_options(${ASAN_OPTIONS})
        message(STATUS "mdcli open asan check")
    else()
        message(STATUS "mdcli close asan check")
    endif()
endif()

if(BUILD_SHARED_LIBS AND (CONAN_PLATFORM MATCHES "x64"))
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
    set(CMAKE_BUILD_RPATH "")
    set(CMAKE_INSTALL_RPATH "")
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()

#log setting
set(DSF_USE_EPROSIMA_LOG ON)
if (DSF_USE_EPROSIMA_LOG)
    message(STATUS "pub-sub use DSF_USE_EPROSIMA_LOG: ${DSF_USE_EPROSIMA_LOG}")
    add_definitions(-DDSF_USE_EPROSIMA_LOG)
endif ()
option(POSIX_SHM "POSIX_SHM" OFF)

# 添加include路径
include_directories(
    ${CMAKE_SOURCE_DIR}/framework/vmw/include
    ${CMAKE_SOURCE_DIR}/evbs/include

    ${PROJECT_SOURCE_DIR}/toolsdk/include
    ${PROJECT_SOURCE_DIR}/common
)

file(GLOB statistic_cli_SOURCES_CPP
    "statistic_cli.cpp"
    "discoverycli/*.cpp"
    "pingcli/*.cpp"
    "record/*.cpp"
    "replay/*.cpp"
    "spy/*.cpp"
)

file(GLOB SDK_SOURCES_CPP
    ${PROJECT_SOURCE_DIR}/toolsdk/src/discoverer/*.cpp
    ${PROJECT_SOURCE_DIR}/toolsdk/src/ping/*.cpp
    ${PROJECT_SOURCE_DIR}/toolsdk/src/recorder/*.cpp
    ${PROJECT_SOURCE_DIR}/toolsdk/src/replayer/*.cpp
    ${PROJECT_SOURCE_DIR}/toolsdk/src/utils/*.cpp
)

set(SQLITE_SRC
    ${VBS_TOP_DIR}/evbs/src/cpp/rtps/persistence/sqlite3.c
)

add_executable(${PROJECT_NAME} ${statistic_cli_SOURCES_CPP} ${SDK_SOURCES_CPP} ${SQLITE_SRC})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${VBS_TOP_DIR}/evbs/src/cpp/rtps/persistence
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
    $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
)

if(CONAN_BUILD)
    target_link_libraries(${PROJECT_NAME} livbs)
else()
    target_link_libraries(${PROJECT_NAME} livbs)
endif()

if (CONAN_BUILD)
    install(TARGETS ${PROJECT_NAME}
        COMPONENT tools
        RUNTIME DESTINATION "${TOOL_INSTALL_DIR}"
    )

    install(PROGRAMS scripts/get_all_mq_info.sh scripts/get_all_pool_info.sh
        COMPONENT tools
        DESTINATION "${TOOL_INSTALL_DIR}"
    )
    
    install(FILES ${PROJECT_SOURCE_DIR}/pingcli/publisher.xml DESTINATION "${TOOL_INSTALL_DIR}/cmd_conf/ping")
    install(FILES ${PROJECT_SOURCE_DIR}/pingcli/subscriber.xml DESTINATION "${TOOL_INSTALL_DIR}/cmd_conf/ping")
    install(FILES ${PROJECT_SOURCE_DIR}/pingcli/rtt_test.xml DESTINATION "${TOOL_INSTALL_DIR}/cmd_conf/ping")

    install(FILES ${PROJECT_SOURCE_DIR}/record/recorder.xml DESTINATION "${TOOL_INSTALL_DIR}/cmd_conf/record")
    install(FILES ${PROJECT_SOURCE_DIR}/replay/replayer.xml DESTINATION "${TOOL_INSTALL_DIR}/cmd_conf/replay")

else()
    install(TARGETS ${PROJECT_NAME}
        COMPONENT tools
        RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/tool/statistic_cli"
    )

    install(PROGRAMS scripts/get_all_mq_info.sh scripts/get_all_pool_info.sh
        COMPONENT tools
        DESTINATION "${CMAKE_INSTALL_PREFIX}/tool/statistic_cli"
    )

    install(FILES ${PROJECT_SOURCE_DIR}/pingcli/publisher.xml DESTINATION "${CMAKE_INSTALL_PREFIX}/tool/statistic_cli/cmd_conf/ping")
    install(FILES ${PROJECT_SOURCE_DIR}/pingcli/subscriber.xml DESTINATION "${CMAKE_INSTALL_PREFIX}/tool/statistic_cli/cmd_conf/ping")
    install(FILES ${PROJECT_SOURCE_DIR}/pingcli/rtt_test.xml DESTINATION "${CMAKE_INSTALL_PREFIX}/tool/statistic_cli/cmd_conf/ping")

    install(FILES ${PROJECT_SOURCE_DIR}/record/recorder.xml DESTINATION "${CMAKE_INSTALL_PREFIX}/tool/statistic_cli/cmd_conf/record")
    install(FILES ${PROJECT_SOURCE_DIR}/replay/replayer.xml DESTINATION "${CMAKE_INSTALL_PREFIX}/tool/statistic_cli/cmd_conf/replay")

endif()