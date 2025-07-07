/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CMDLINECOM_HPP_
#define CMDLINECOM_HPP_
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>

void typeDiscoveryCli(const std::vector<std::string>& params);
void pingCli(const std::vector<std::string>& params);
void recordCli(const std::vector<std::string>& params);
void replayCli(const std::vector<std::string>& params);
void spyCli(const std::vector<std::string>& params);
void freeargv(std::vector<char*>& argv);
bool checkParam(const std::vector<std::string>& params);
#endif
