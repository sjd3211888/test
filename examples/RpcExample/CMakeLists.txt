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

cmake_minimum_required(VERSION 3.16.3)

project(RpcExample VERSION 1 LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -rdynamic")

message(STATUS "Configuring RpcExample...")

set(VBS_RPC_CLIENT RpcDemoClient)
set(VBS_RPC_SERVER RpcDemoServer)

set(VBS_RPC_CLIENT_SOURCE
    RpcDemo.cxx
    RpcDemoClientExample.cxx
)

set(VBS_RPC_SERVER_SOURCE
    RpcDemo.cxx
    RpcDemoServerImplExample.cxx
    RpcDemoServerExample.cxx
)

add_executable(${VBS_RPC_CLIENT} ${VBS_RPC_CLIENT_SOURCE})
add_executable(${VBS_RPC_SERVER} ${VBS_RPC_SERVER_SOURCE})

target_link_libraries(${VBS_RPC_CLIENT} livbs ecdr)
target_link_libraries(${VBS_RPC_SERVER} livbs ecdr)

target_include_directories(${VBS_RPC_CLIENT} PUBLIC
    $<TARGET_PROPERTY:livbs,INTERFACE_INCLUDE_DIRECTORIES>
)

target_include_directories(${VBS_RPC_SERVER} PUBLIC
    $<TARGET_PROPERTY:livbs,INTERFACE_INCLUDE_DIRECTORIES>
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/RpcDemoXML.xml
               ${CMAKE_INSTALL_PREFIX}/examples/rpc/RpcDemoXML.xml
               COPYONLY)

install(TARGETS ${VBS_RPC_CLIENT} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/rpc)
install(TARGETS ${VBS_RPC_SERVER} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/rpc)
