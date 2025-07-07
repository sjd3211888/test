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
cmake_minimum_required(VERSION 3.16)
# only build for linux
project(vbstoolsdk LANGUAGES C CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES_CPP
    ${PROJECT_SOURCE_DIR}/src/recorder/*.cpp
    ${PROJECT_SOURCE_DIR}/src/replayer/*.cpp
    ${PROJECT_SOURCE_DIR}/src/discoverer/*.cpp
    ${PROJECT_SOURCE_DIR}/src/ping/*.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/*.cpp
)

set(SQLITE_SRC
    ${VBS_TOP_DIR}/evbs/src/cpp/rtps/persistence/sqlite3.c
)

message(STATUS "Configuring vbstoolsdk...")

set(SOURCE_FILE_GROUP
    ${SOURCES_CPP}
    ${SQLITE_SRC}
)

if(BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILE_GROUP})
else()
    add_library(${PROJECT_NAME} STATIC ${SOURCE_FILE_GROUP})
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES INCLUDE_DIRECTORIES "")
set_source_files_properties(
    ${SQLITE_SRC}
    PROPERTIES COMPILE_OPTIONS "-fvisibility=hidden"
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    livbs
    boost_filesystem
    boost_wave
    boost_thread
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CONAN_INCLUDE_DIRS_BOOST}
    ${VBS_TOP_DIR}/framework/vui/include
    ${VBS_TOP_DIR}/framework/vmw/include
    ${VBS_TOP_DIR}/utils/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${VBS_TOP_DIR}/evbs/src/cpp/rtps/persistence
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/discoverer DESTINATION ${CMAKE_INSTALL_PREFIX}/include/vbstoolsdk)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/utils DESTINATION ${CMAKE_INSTALL_PREFIX}/include/vbstoolsdk)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ping DESTINATION ${CMAKE_INSTALL_PREFIX}/include/vbstoolsdk)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/recorder DESTINATION ${CMAKE_INSTALL_PREFIX}/include/vbstoolsdk)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/replayer DESTINATION ${CMAKE_INSTALL_PREFIX}/include/vbstoolsdk)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
