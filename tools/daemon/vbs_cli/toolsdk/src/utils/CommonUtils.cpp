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

#include <string>
#include <cstring>
#include <array>
#include <fstream>
#include <sstream>
#include <elog/Log.hpp>
#include "utils/CommonUtils.hpp"

namespace vbstoolsdk {

bool run_script_check_error(const std::string& cmd) {
    std::array<char, 128> buffer;
    bool error_flag = false;
    logInfo_("VBSTOOLSDK", "cmd: " << cmd);
    std::string error_output;
#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) {
        logError_("VBSTOOLSDK", "can not open pipe");
        return true;
    }

    try {
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            if (strstr(buffer.data(), "error")) {
                logError_("VBSTOOLSDK", "cmd: " << cmd << " error");
                error_flag = true;
                error_output += buffer.data();
            }
        }
    } catch (const std::exception& e) {
        logError_("VBSTOOLSDK", e.what());
        _pclose(pipe);
        return true;
    }

    int exit_status = _pclose(pipe);
#else
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        logError_("VBSTOOLSDK", "can not open pipe");
        return true;
    }

    try {
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            if (strstr(buffer.data(), "error")) {
                error_flag = true;
                error_output += buffer.data();
            }
        }
    } catch (const std::exception& e) {
        logError_("VBSTOOLSDK", e.what());
        pclose(pipe);
        return true;
    }

    int exit_status = pclose(pipe);
#endif
    if (error_flag == true) {
        logError_("VBSTOOLSDK", error_output);
        return true;
    }
    return exit_status != 0;
}

}  // namespace vbstoolsdk
