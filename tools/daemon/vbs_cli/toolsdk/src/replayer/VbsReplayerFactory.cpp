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

#include <filesystem>
#include <elog/Log.hpp>
#include "replayer/VbsReplayerFactory.hpp"

namespace vbstoolsdk {

std::unique_ptr<VbsReplayer> VbsReplayerFactory::creater_replayer(const std::string& path,
                                                                  const std::string& xml_file_name,
                                                                  const std::string& participant_profile,
                                                                  const std::string& writer_profile, int domain_id,
                                                                  bool rti_flag) {
    std::filesystem::path file_path(path);
    std::string extension = file_path.extension().string();
    if (extension == ".dat") {
        return std::make_unique<VbsDatReplayer>(path, xml_file_name, participant_profile, writer_profile, domain_id,
                                                rti_flag);
    } else if (extension == ".Pack") {
        return nullptr;
    } else {
        logError_("VBSTOOLSDK", "now replayer not support: " << extension);
        return nullptr;
    }
}

}  // namespace vbstoolsdk
