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

#ifndef DSFDDS_DDS_LOG_LOG_HPP_
#define DSFDDS_DDS_LOG_LOG_HPP_

#include <memory>
#include <string>
#include <cstdint>
#include "elog/FileConsumer.hpp"
#include "elog/Log.hpp"
#include "elog/OStreamConsumer.hpp"
#include "elog/StdoutConsumer.hpp"
#include "elog/StdoutErrConsumer.hpp"

class dsflog {
 public:
    /**
   * @param filename log to file name
   * @param out_to_file if true out log to filename file, default false, only out to console
   */
    inline static void init_dsf_log(const std::string& filename = "", bool out_to_console = true);
};

#ifndef DSF_LOG_INFO
#define DSF_LOG_INFO(cat, msg) logInfo_(DSF_##cat, msg)
#endif

#ifndef DSF_LOG_WARNING
#define DSF_LOG_WARNING(cat, msg) logWarning_(DSF_##cat, msg)
#endif

#ifndef DSF_LOG_ERROR
#define DSF_LOG_ERROR_2_ARGS(cat, msg) logError_(cat, msg)
#define DSF_LOG_ERROR_3_ARGS(cat, code, msg) \
    DSF_LOG_ERROR_2_ARGS(cat, vbsutil::elog::decToHex(DSF_ERRORCODE(ExCode_t::cat, code)) << " " << msg)
#define DSF_LOG_ERROR(...) DSF_LOG_ERROR_X_ARGS(__VA_ARGS__, DSF_LOG_ERROR_3_ARGS, DSF_LOG_ERROR_2_ARGS)(__VA_ARGS__)
#define DSF_LOG_ERROR_X_ARGS(_1, _2, _3, NAME, ...) NAME
#endif

#ifndef DSF_LOG_EVENT
#define DSF_LOG_EVENT(cat, msg) logEvent_(DSF_##cat, msg)
#endif

#ifndef DSF_LOG_DEBUG
#define DSF_LOG_DEBUG(cat, msg) logDebug_(DSF_##cat, msg)
#endif

#define DSF_LOG_ERROR_KEY_T(cat, code, key, msg) ELOG_DSF_EVERY_T_WITH_KEY_(cat, Error, code, key, msg)

#define DSF_LOG_ERROR_T(cat, code, msg) ELOG_DSF_EVERY_T_(cat, Error, code, msg)

#define DSF_LOG_WARNING_KEY_T(cat, code, key, msg) ELOG_DSF_EVERY_T_WITH_KEY_(cat, Warning, code, key, msg)

#define DSF_LOG_WARNING_T(cat, code, msg) ELOG_DSF_EVERY_T_(cat, Warning, code, msg)

#define DSF_LOG_INFO_KEY_T(cat, code, key, msg) ELOG_DSF_EVERY_T_WITH_KEY_(cat, Info, code, key, msg)

#define DSF_LOG_INFO_T(cat, code, msg) ELOG_DSF_EVERY_T_(cat, Info, code, msg)

inline void dsflog::init_dsf_log(const std::string& filename, bool out_to_console) {
    using vbsutil::elog::FileConsumer;
    using vbsutil::elog::Log;
    using vbsutil::elog::LogConsumer;

    Log::SetVerbosity(Log::Error);
    Log::ReportFunctions(true);
    if (out_to_console) {
        Log::ReportFilenames(false);
    } else {
        Log::ClearConsumers();
    }
    if (!filename.empty()) {
        Log::ReportFilenames(true);
        Log::RegisterConsumer(std::make_unique<FileConsumer>(filename));
    }
}

#endif  // ifndef _DTSDDS_DDS_LOG_LOG_HPP_
