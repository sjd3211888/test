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
###############################################################################
# CMake build rules for EXAMPLE
###############################################################################
cmake_minimum_required(VERSION 3.6)

project(TransportExample VERSION 1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SOURCES_CXX "*.cxx")
file(GLOB SOURCES_CPP "*.cpp")

configure_file(${PROJECT_SOURCE_DIR}/test_xml_pub.xml
        ${CMAKE_INSTALL_PREFIX}/examples/test_xml_pub.xml
        COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/test_xml_sub.xml
        ${CMAKE_INSTALL_PREFIX}/examples/test_xml_sub.xml
        COPYONLY)

include_directories(${PROJECT_SOURCE_DIR})

message(STATUS "Configuring HelloWorld...")

add_executable(${PROJECT_NAME} ${SOURCES_CXX} ${SOURCES_CPP})

target_link_libraries(${PROJECT_NAME} livbs ecdr)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<TARGET_PROPERTY:livbs,INTERFACE_INCLUDE_DIRECTORIES>
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/examples)