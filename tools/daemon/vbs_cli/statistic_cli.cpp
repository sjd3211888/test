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

#ifdef _WIN32
#include <winsock2.h>
#endif
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#ifndef _WIN32
#include <termios.h>
#endif
#include <cstring>
#include <cstdlib>
#include <elog/Log.hpp>
#include <builtin/lookup/StatisticLookup.hpp>
#include <ertps/xmlparser/XMLProfileManager.h>
#include <transport/UDPv4TransportDescriptor.h>
#ifndef TOOL_ANDROID
#include "cmdlineCom.hpp"
#endif
#include <csignal>
#include <stdexcept>

using vbsutil::elog::Log;
using namespace evbs::ertps::rtps;
using namespace evbs::edds::dds;

#define LOG(str) std::cout << str << std::endl

std::vector<std::string> inputHistory;
int historyIndex = -1;

void freeargv(std::vector<char*>& argv) {
    if (argv.empty()) {
        return;
    }
    for (char*& ptr : argv) {
        if (ptr != nullptr) {
            free(ptr);
            ptr = nullptr;
        }
    }
    argv.clear();
}

// 保存和恢复终端设置
class TerminalSettings {
 public:
    TerminalSettings() {
        tcgetattr(STDIN_FILENO, &oldSettings);
        newSettings = oldSettings;
        newSettings.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);
    }

    ~TerminalSettings() { tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings); }

 private:
    struct termios oldSettings, newSettings;
};

static std::string time_convert(std::int64_t ns) {
    std::stringstream out;
    // 将纳秒值转换为 chrono::time_point
    std::chrono::nanoseconds ns_since_epoch(ns);

    // 使用 duration_cast 将 nanoseconds 转换为 system_clock 的 duration 类型
    std::chrono::time_point<std::chrono::system_clock> tp(
        std::chrono::duration_cast<std::chrono::system_clock::duration>(ns_since_epoch));

    // 转换 time_point 到 time_t 并获得毫秒值
    std::time_t time = std::chrono::system_clock::to_time_t(tp);
    auto ms_part = std::chrono::duration_cast<std::chrono::milliseconds>(ns_since_epoch).count() % 1000;

    // 使用标准库函数将 time_t 转换为日期和时间
    std::tm* tm = std::localtime(&time);

    // 打印日期和时间
    out << std::put_time(tm, "%Y-%m-%d %H:%M:%S");

    // 打印毫秒值
    out << "." << std::setw(3) << std::setfill('0') << ms_part;
    return out.str();
}

static std::string get_hostname() {
#ifdef _WIN32
    return "unknown";
#else
    char hostname[256] = {0};
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        return "unknown";
    }
    return hostname;
#endif
}

static std::vector<std::string> parseIPs(const std::string& input) {
    size_t start = input.find('[');
    size_t end = input.find(']');
    std::vector<std::string> serverLocatorIp;
    if (start != std::string::npos && end != std::string::npos && start < end) {
        // 提取方括号之间的内容
        std::string content = input.substr(start + 1, end - start - 1);

        // 使用字符串流来分割内容
        std::stringstream ss(content);
        std::string token;
        while (std::getline(ss, token, ',')) {
            size_t first = token.find_first_not_of(' ');
            size_t last = token.find_last_not_of(' ');
            if (first != std::string::npos && last != std::string::npos) {
                token = token.substr(first, (last - first + 1));
                serverLocatorIp.push_back(token);
            }
        }
    }
    return serverLocatorIp;
}

static const std::string PROMPT = ">> ";  // 定义前缀

// 重绘整行并定位光标
void redrawLine(uint64_t newCursorPos, const std::string& input) {
    std::cout << "\r" << PROMPT << input << "\x1b[0K";  // 回到行首并输出完整内容
    uint64_t moveBack = input.length() - newCursorPos;
    if (moveBack > 0) {
        std::cout << "\x1b[" << moveBack << "D";  // 光标回位
    }
}

// 更新历史命令
void updateInputWithHistory(uint64_t& cursorPos, std::string& currentInput, uint64_t historyIndex) {
    currentInput = inputHistory[historyIndex];
    redrawLine(currentInput.length(), currentInput);
    cursorPos = currentInput.length();
}

// 清空当前行
void clearCurrentLine(uint64_t& cursorPos, std::string& currentInput) {
    currentInput.clear();
    redrawLine(PROMPT.length(), currentInput);
    cursorPos = PROMPT.length();
}

static void handleInputKeys(std::string& currentInput) {
    TerminalSettings termSettings;
    uint64_t cursorPos = 0;                       // 使用uint64_t类型
    uint64_t historyIndex = inputHistory.size();  // 初始化为最新历史

    std::cout << PROMPT;
    while (true) {
        char ch = getchar();

        if (ch == '\033') {  // ESC 键开头，可能是方向键
            getchar();       // 跳过 [
            ch = getchar();
            if (ch == 'A') {  // 上箭头键 - 历史记录
                if (historyIndex > 0) {
                    historyIndex--;
                    updateInputWithHistory(cursorPos, currentInput, historyIndex);
                }
            } else if (ch == 'B') {  // 下箭头键 - 历史记录
                if (historyIndex < inputHistory.size()) {
                    historyIndex++;
                    if (historyIndex == inputHistory.size()) {
                        clearCurrentLine(cursorPos, currentInput);
                    } else {
                        updateInputWithHistory(cursorPos, currentInput, historyIndex);
                    }
                }
            } else if (ch == 'C') {  // 右箭头键 - 光标右移
                if (cursorPos < currentInput.length()) {
                    std::cout << "\x1b[1C";  // 光标右移
                    cursorPos++;
                }
            } else if (ch == 'D') {  // 左箭头键 - 光标左移
                if (cursorPos > 0) {
                    std::cout << "\x1b[1D";  // 光标左移
                    cursorPos--;
                }
            } else if (ch == '3') {  // Delete键
                if (getchar() == '~') {
                    currentInput.erase(cursorPos, 1);
                    redrawLine(cursorPos, currentInput);
                }
            }
        } else if (ch == '\n') {  // 回车键
            if (!currentInput.empty()) {
                inputHistory.push_back(currentInput);
                historyIndex = inputHistory.size();
            }
            std::cout << std::endl;
            break;
        } else if ((ch == 8 || ch == 127) && cursorPos > 0) {  // 退格键
            currentInput.erase(cursorPos - 1, 1);
            redrawLine(cursorPos - 1, currentInput);
            cursorPos--;
        } else if (isprint(ch)) {  // 可打印字符
            currentInput.insert(cursorPos, 1, ch);
            redrawLine(cursorPos + 1, currentInput);
            cursorPos++;
        }
    }
}

bool checkParam(const std::vector<std::string>& params) {
    if (params.size() == 0) {
        return false;
    }
    for (size_t i = 0; i < params.size(); i++) {
        if (params[i] == "?" || params[i] == "-h" || params[i] == "-help" || params[i] == "--h" ||
            params[i] == "--help") {
            return false;
        }
    }
    return true;
}

static void lookupParticipantRow(uint32_t domain_id, int32_t pid, uint32_t timeout) {
    auto infos = vbs::builtin::StatisticLookup::get_instance()->lookupParticipantsByDomain(domain_id, timeout);
    if (infos.size() > 0) {
        uint32_t valid_cnt = 1;
        LOG("================Participant Dump================");
        for (uint32_t i = 0; i < infos.size(); i++) {
            if (pid >= 0 && (infos[i].info.m_guid.get_process_id() != pid)) {
                continue;
            }
            LOG("index  :" << valid_cnt++);
            LOG("alive  :" << infos[i].info.isAlive);
            LOG("name   :" << infos[i].info.m_participantName);
            LOG("guid   :" << infos[i].info.m_guid);
            std::string versionStr = std::to_string(infos[i].info.m_evbsRemoteVersion[0]) + "." +
                                     std::to_string(infos[i].info.m_evbsRemoteVersion[1]) + "." +
                                     std::to_string(infos[i].info.m_evbsRemoteVersion[2]) + "." +
                                     std::to_string(infos[i].info.m_evbsRemoteVersion[3]);
            LOG("vertion:" << versionStr);
            LOG("status :" << infos[i].status);
            LOG("meta-locators unicast  :");
            for (auto& lo : infos[i].info.metatraffic_locators.unicast) {
                LOG("    " << lo);
            }
            LOG("meta-locators multicast:");
            for (auto& lo : infos[i].info.metatraffic_locators.multicast) {
                LOG("    " << lo);
            }
            LOG("default unicast        :");
            for (auto& lo : infos[i].info.default_locators.unicast) {
                LOG("    " << lo);
            }
            LOG("default multicast      :");
            for (auto& lo : infos[i].info.default_locators.multicast) {
                LOG("    " << lo);
            }
            LOG("leaseDuration          :" << infos[i].info.m_leaseDuration);
            LOG("startTime              :" << time_convert(infos[i].info.m_startTime.to_ns()));
            LOG("hostName               :" << infos[i].info.m_hostName);
            LOG("------------------------------------------------");
        }
    } else {
        LOG("lookup participant empty.");
    }
}

static void lookupParticipantCol(uint32_t domain_id, int32_t pid, uint32_t timeout) {
    auto infos = vbs::builtin::StatisticLookup::get_instance()->lookupParticipantsByDomain(domain_id, timeout);
    if (infos.size() == 0) {
        LOG("participant empty.");
        return;
    }

    // 初始化各列的最大宽度
    size_t nameWidth = 0;
    size_t versionWidth = 0;
    size_t startTimeWidth = 0;
    size_t hostNameWidth = 0;
    size_t guidWidth = 0;

    LOG("=================Participant Dump===============");
    // 计算各列的最大宽度
    for (const auto& info : infos) {
        if (pid >= 0 && (info.info.m_guid.get_process_id() != pid)) {
            continue;
        }
        if (!info.info.isAlive ||
            info.status != evbs::ertps::rtps::ParticipantDiscoveryInfo::DISCOVERY_STATUS::DISCOVERED_PARTICIPANT) {
            continue;
        }
        nameWidth = std::max(nameWidth, info.info.m_participantName.size());
        std::string versionStr = std::to_string(info.info.m_evbsRemoteVersion[0]) + "." +
                                 std::to_string(info.info.m_evbsRemoteVersion[1]) + "." +
                                 std::to_string(info.info.m_evbsRemoteVersion[2]) + "." +
                                 std::to_string(info.info.m_evbsRemoteVersion[3]);
        versionWidth = std::max(versionWidth, versionStr.length());
        std::string startTimeStr = time_convert(info.info.m_startTime.to_ns());
        startTimeWidth = std::max(startTimeWidth, startTimeStr.length());
        hostNameWidth = std::max(hostNameWidth, info.info.m_hostName.size());
        std::stringstream guid_stream;
        guid_stream << info.info.m_guid;
        guidWidth = std::max(guidWidth, guid_stream.str().length());
    }

    // 输出participant信息表头
    LOG(std::left << std::setw(nameWidth + 2) << "Participant" << std::setw(versionWidth + 2) << "Version"
                  << std::setw(startTimeWidth + 2) << "Start Time" << std::setw(hostNameWidth + 2) << "Host Name"
                  << std::setw(guidWidth + 2) << "Guid");

    // 输出participant信息
    for (const auto& info : infos) {
        if (pid >= 0 && (info.info.m_guid.get_process_id() != pid)) {
            continue;
        }

        if (!info.info.isAlive ||
            info.status != evbs::ertps::rtps::ParticipantDiscoveryInfo::DISCOVERY_STATUS::DISCOVERED_PARTICIPANT) {
            continue;
        }
        std::string versionStr = std::to_string(info.info.m_evbsRemoteVersion[0]) + "." +
                                 std::to_string(info.info.m_evbsRemoteVersion[1]) + "." +
                                 std::to_string(info.info.m_evbsRemoteVersion[2]) + "." +
                                 std::to_string(info.info.m_evbsRemoteVersion[3]);
        std::string startTimeStr = time_convert(info.info.m_startTime.to_ns());

        LOG(std::left << std::setw(nameWidth + 2) << info.info.m_participantName << std::setw(versionWidth + 2)
                      << versionStr << std::setw(startTimeWidth + 2) << startTimeStr << std::setw(hostNameWidth + 2)
                      << info.info.m_hostName << info.info.m_guid);
    }
    std::cout << "------------------------------------------------" << std::endl;
}

static void lookupParticipant(const std::vector<std::string>& params) {
    if (checkParam(params) == false) {
        LOG("Usage: part <domain> [pid:-1] [show_by_row:0]");
        return;
    }

    uint32_t timeout = VBS_TOOL_WAIT_TIMEOUT;
    uint32_t domain_id = atoi(params[0].c_str());
    int32_t pid = -1;
    bool detail = false;
    if (params.size() >= 2) {
        pid = atoi(params[1].c_str());
        LOG("target pid: " << pid);
    }
    if (params.size() >= 3) {
        detail = atoi(params[2].c_str());
        LOG("show by row.");
    }
    if (detail) {
        lookupParticipantRow(domain_id, pid, timeout);
    } else {
        lookupParticipantCol(domain_id, pid, timeout);
    }
}

static void lookupParticipantDetail(const std::vector<std::string>& params) {
    if (checkParam(params) == false) {
        LOG("Usage: part-detail <domain> <guid> <timeout>");
        return;
    }

    try {
        uint32_t timeout = VBS_TOOL_WAIT_TIMEOUT;
        uint32_t domain_id = atoi(params[0].c_str());
        std::istringstream guid_input(params[1]);
        evbs::ertps::rtps::GUID_t guid;
        guid_input >> guid;
        if (params.size() > 2) {
            timeout = atoi(params[2].c_str());
        }
        auto infos =
            vbs::builtin::StatisticLookup::get_instance()->lookupParticipantDetailInfo(domain_id, guid, timeout);
        if (infos.size() > 0) {
            LOG("=============participant detail info============");
            for (uint32_t i = 0; i < infos.size(); i++) {
                LOG("count     :" << i);
                LOG("kind      :" << (infos[i].type == builtin::StatisticEntityType::STATICTIC_ENTITY_WRITER
                                          ? "Writer"
                                          : "Reader"));
                LOG("builtin   :" << (infos[i].guid.is_builtin() ? "true" : "false"));
                LOG("topic name:" << infos[i].topic_name);
                LOG("type  name:" << infos[i].type_name);
                LOG("guid      :" << infos[i].guid);
                LOG("matched   :" << infos[i].matched);
                LOG("------------------------------------------------");
            }
        } else {
            LOG("lookup participant detail empty.");
        }
    } catch (...) {
        LOG("Usage: part-detail <domain> <guid> <timeout>");
    }
}

static void lookupTopic(const std::vector<std::string>& params) {
    if (params.size() < 2 || !checkParam(params)) {
        LOG("Usage: topic <domain> <topic_name> <timeout>");
        return;
    }

    uint32_t timeout = VBS_TOOL_WAIT_TIMEOUT;
    uint32_t domain_id = atoi(params[0].c_str());
    std::string topic_name = params[1].c_str();
    if (params.size() == 3) {  // domainid topic_name [timeout]
        timeout = atoi(params[2].c_str());
    }
    auto infos = vbs::builtin::StatisticLookup::get_instance()->lookupParticipantsByDomain(domain_id, timeout);
    if (infos.size() == 0) {
        LOG("participant not found.");
        return;
    }
    LOG("====================guid info===================");
    for (uint32_t i = 0; i < infos.size(); i++) {
        if (!infos[i].info.isAlive) {
            continue;
        }
        auto detailInfos = vbs::builtin::StatisticLookup::get_instance()->lookupParticipantDetailInfo(
            domain_id, infos[i].info.m_guid, timeout);
        if (detailInfos.size() > 0) {
            for (uint32_t j = 0; j < detailInfos.size(); j++) {
                if (detailInfos[j].topic_name == topic_name) {
                    LOG("participant name   :" << infos[i].info.m_participantName);
                    LOG("kind               :"
                        << (detailInfos[j].type == builtin::StatisticEntityType::STATICTIC_ENTITY_WRITER ? "Writer"
                                                                                                         : "Reader"));
                    LOG("type  name         :" << detailInfos[j].type_name);
                    LOG("guid               :" << detailInfos[j].guid);
                    LOG("------------------------------------------------");
                }
            }
        }
    }
}

static void lookupMatchInfo(const std::vector<std::string>& params) {
    if (params.size() <= 1 || !checkParam(params)) {
        LOG("Usage: match <domain> <guid> <timeout>");
        return;
    }

    try {
        uint32_t domain_id = atoi(params[0].c_str());
        std::istringstream guid_input(params[1]);
        evbs::ertps::rtps::GUID_t guid;
        guid_input >> guid;
        uint32_t timeout = VBS_TOOL_WAIT_TIMEOUT;
        if (params.size() > 2) {
            timeout = atoi(params[2].c_str());
        }
        auto infos = vbs::builtin::StatisticLookup::get_instance()->lookupMatchGuidsInfo(domain_id, guid, timeout);
        if (infos.size() > 0) {
            LOG("===============matched guid info================");
            for (uint32_t i = 0; i < infos.size(); i++) {
                LOG("count     :" << i);
                LOG("guid      :" << infos[i]);
                LOG("------------------------------------------------");
            }
        } else {
            LOG("lookup matched guids empty.");
        }
    } catch (...) {
        LOG("Usage: match <domain> <guid> <timeout>");
    }
}

static void lookupSendInfo(const std::vector<std::string>& params) {
    if (params.size() <= 1 || !checkParam(params)) {
        LOG("Usage: send <domain> <guid> <timeout>");
        return;
    }
    try {
        uint32_t domain_id = atoi(params[0].c_str());
        std::istringstream guid_input(params[1]);
        evbs::ertps::rtps::GUID_t guid;
        guid_input >> guid;
        uint32_t timeout = VBS_TOOL_WAIT_TIMEOUT;
        if (params.size() > 2) {
            timeout = atoi(params[2].c_str());
        }
        builtin::StatisticSendInfo info;
        auto ret = vbs::builtin::StatisticLookup::get_instance()->lookupWriterSendInfo(domain_id, guid, &info, timeout);
        if (ret == ReturnCode_t::RETCODE_OK) {
            LOG("===================send info====================");
            LOG("send_count :" << info.send_count);
            LOG("send_throughput    :" << info.send_throughput);
            LOG("send_payload_length_avg   :" << info.send_payload_length_avg);
            LOG("send_last_pkt_time   :" << time_convert(info.last_pkt_timestamp));
            LOG("send_first_pkt_time   :" << time_convert(info.first_pkt_timestamp));
        }
    } catch (...) {
        LOG("Usage: send <domain> <guid> <timeout>");
    }
}

static void lookupRecvInfo(const std::vector<std::string>& params) {
    if (params.size() <= 1 || !checkParam(params)) {
        LOG("Usage: recv <domain> <guid> <timeout>");
        return;
    }
    try {
        uint32_t domain_id = atoi(params[0].c_str());
        std::istringstream guid_input(params[1]);
        evbs::ertps::rtps::GUID_t guid;
        guid_input >> guid;
        uint32_t timeout = VBS_TOOL_WAIT_TIMEOUT;
        if (params.size() > 2) {
            timeout = atoi(params[2].c_str());
        }
        builtin::StatisticRecvInfo info;
        auto ret = vbs::builtin::StatisticLookup::get_instance()->lookupReaderRecvInfo(domain_id, guid, &info, timeout);
        if (ret == ReturnCode_t::RETCODE_OK) {
            LOG("===================recv info====================");
            LOG("recv_count :");
            for (auto& it : info.recv_count) {
                LOG("   guid :" << it.first << " count:" << it.second);
            }
            LOG("lost_count :");
            for (auto& it : info.lost_count) {
                LOG("   guid :" << it.first << " count:" << it.second);
            }
            LOG("avg_latancy :");
            for (auto& it : info.avg_latancy) {
                LOG("   guid :" << it.first << " latancy:" << it.second);
            }
            LOG("last_pkt_time:");
            for (auto& it : info.last_pkt_timestamp) {
                LOG("   guid :" << it.first << " lastpkt_time:" << time_convert(it.second));
            }
            LOG("first_pkt_time:");
            for (auto& it : info.first_pkt_timestamp) {
                LOG("   guid :" << it.first << " firstpkt_time:" << time_convert(it.second));
            }
            LOG("take_count_total :" << info.take_count_total);
            LOG("untake_count     :" << info.untake_count);
            LOG("rejected_count   :" << info.rejected_count);
        }
    } catch (...) {
        LOG("Usage: recv <domain> <guid> <timeout>");
    }
}

static void lookupQosInfo(const std::vector<std::string>& params) {
    if (params.size() <= 1 || !checkParam(params)) {
        LOG("Usage: qos <domain> <guid> <timeout>");
        return;
    }

    try {
        uint32_t domain_id = atoi(params[0].c_str());
        std::istringstream guid_input(params[1]);
        evbs::ertps::rtps::GUID_t guid;
        guid_input >> guid;
        uint32_t timeout = VBS_TOOL_WAIT_TIMEOUT;
        if (params.size() > 2) {
            timeout = atoi(params[2].c_str());
        }
        builtin::StatisticWriterReaderQos qos;
        auto ret = vbs::builtin::StatisticLookup::get_instance()->lookupWriterReaderQos(domain_id, guid, &qos, timeout);
        if (ret == ReturnCode_t::RETCODE_OK) {
            LOG("=============writer reader qos info=============");
            LOG("durability:" << qos.durability);
            LOG("deadline:" << qos.deadline);
            LOG("liveliness_kind:" << qos.liveliness_kind);
            LOG("liveliness_lease_duration:" << qos.liveliness_lease_duration);
            LOG("liveliness_announcement_period:" << qos.liveliness_announcement_period);
            LOG("reliability_kind:" << qos.reliability_kind);
            LOG("reliability_max_blocking_time:" << qos.reliability_max_blocking_time);
            LOG("history_kind:" << qos.history_kind);
            LOG("history_depth:" << qos.history_depth);
            LOG("max_samples:" << qos.max_samples);
            LOG("max_instances:" << qos.max_instances);
            LOG("max_samples_per_instance:" << qos.max_samples_per_instance);
            LOG("allocated_samples:" << qos.allocated_samples);
            LOG("extra_samples:" << qos.extra_samples);
            LOG("lifespan:" << qos.lifespan);
            LOG("ownership:" << qos.ownership);
            if (guid.entityId.is_writer()) {
                LOG("pub_mode:" << qos.pub_mode);
                LOG("flowctlname:" << qos.flowctlname);
            }
            LOG("e2e_enable:" << qos.e2e_enable);
            LOG("send_multi_enable:" << qos.send_multi_enable);
        }
    } catch (...) {
        LOG("Usage: qos <domain> <guid> <timeout>");
    }
}

static void lookupMessageBriefInfo(const std::vector<std::string>& params) {
    if (params.size() <= 2 || !checkParam(params)) {
        LOG("msgbrief <domain> <pid> <topic> <timeout>");
        return;
    }

    try {
        uint32_t domain_id = atoi(params[0].c_str());
        uint32_t pid = atoi(params[1].c_str());
        std::string topic_name(params[2]);
        std::string host_name;
        uint32_t timeout = VBS_TOOL_WAIT_TIMEOUT;
        if (params.size() > 3) {
            host_name = atoi(params[3].c_str());
        } else {
            host_name = get_hostname();
        }
        if (params.size() > 4) {
            timeout = atoi(params[4].c_str());
        }
        Log::GuidMessageBriefsMap msg_briefs_map = vbs::builtin::StatisticLookup::get_instance()->lookupMessageBrief(
            domain_id, host_name, pid, topic_name, timeout);
        if (!msg_briefs_map.empty()) {
            std::stringstream output;
            output << "Message briefs. pid:" << pid << " Topic:" << topic_name << std::endl;
            for (const auto& guid_msg_briefs_it : msg_briefs_map) {
                output << "Writer guid: " << guid_msg_briefs_it.first << std::endl;
                for (const auto& msg_brief : guid_msg_briefs_it.second) {
                    output << (msg_brief.is_writer ? "Send" : "Recv") << " Src-time "
                           << time_convert(msg_brief.source_timestamp) << " Dst-time "
                           << time_convert(msg_brief.dst_timestamp) << " Seq " << msg_brief.sequence << " Length "
                           << msg_brief.length << std::endl;
                }
            }
            LOG("=================msgbrief info==================");
            LOG(output.str());
        } else {
            LOG("Pid " << pid << " topic " << topic_name << " result: empty.");
        }
    } catch (...) {
        LOG("msgbrief <domain> <pid> <topic> <timeout>");
    }
}

static void lookupProxyInfo(const std::vector<std::string>& params) {
    if (params.size() <= 1 || !checkParam(params)) {
        LOG("Usage: proxy <domain> <guid> <timeout>");
        return;
    }
    try {
        uint32_t domain_id = atoi(params[0].c_str());
        std::istringstream guid_input(params[1]);
        evbs::ertps::rtps::GUID_t guid;
        guid_input >> guid;
        uint32_t timeout = VBS_TOOL_WAIT_TIMEOUT;
        if (params.size() > 2) {
            timeout = atoi(params[2].c_str());
        }
        auto infos =
            vbs::builtin::StatisticLookup::get_instance()->lookupWriterReaderProxyInfos(domain_id, guid, timeout);
        if (infos.size() > 0) {
            LOG("=================proxy info=================");
            for (uint32_t i = 0; i < infos.size(); i++) {
                LOG("count     :" << i);
                LOG("type      :" << (infos[i].type == builtin::STATICTIC_ENTITY_WRITER ? "writer proxy"
                                                                                        : "reader proxy"));
                LOG("is_alive  :" << (infos[i].is_alive ? "true" : "false"));
                LOG("start time:" << time_convert(infos[i].start_time));
                if (infos[i].type == builtin::STATICTIC_ENTITY_WRITER) {
                    LOG("remote_guid          :" << infos[i].writer_proxy_locators_entry.remote_guid);
                    LOG("locator enable       :" << (infos[i].writer_proxy_locators_entry.enabled ? "true" : "false"));
                    LOG("locator unicast      :");
                    for (auto& lo : infos[i].writer_proxy_locators_entry.unicast) {
                        LOG("    " << lo);
                    }
                    LOG("locator multicast    :");
                    for (auto& lo : infos[i].writer_proxy_locators_entry.multicast) {
                        LOG("    " << lo);
                    }
                    LOG("last_heartbeat_count :" << infos[i].last_heartbeat_count);
                    LOG("last_notified        :" << infos[i].last_notified);
                    LOG("available_changes_max:" << infos[i].available_changes_max);
                    LOG("max_sequence_number  :" << infos[i].max_sequence_number);
                } else {
                    LOG("last_acknack_count   :" << infos[i].last_acknack_count);
                    LOG("last_nackfrag_count  :" << infos[i].last_nackfrag_count);
                    LOG("changes_low_mark     :" << infos[i].changes_low_mark);
                    LOG("remote_guid          :" << infos[i].reader_proxy_general_locator_entry.remote_guid);
                    LOG("locator enable       :" << (infos[i].reader_proxy_general_locator_entry.enabled ? "true"
                                                                                                         : "false"));
                    LOG("locator unicast      :");
                    for (auto& lo : infos[i].reader_proxy_general_locator_entry.unicast) {
                        LOG("    " << lo);
                    }
                    LOG("locator multicast    :");
                    for (auto& lo : infos[i].reader_proxy_general_locator_entry.multicast) {
                        LOG("    " << lo);
                    }
                    LOG("async remote_guid    :" << infos[i].reader_proxy_async_locator_entry.remote_guid);
                    LOG("async locator enable :" << (infos[i].reader_proxy_async_locator_entry.enabled ? "true"
                                                                                                       : "false"));
                    LOG("async locator unicast:");
                    for (auto& lo : infos[i].reader_proxy_async_locator_entry.unicast) {
                        LOG("    " << lo);
                    }
                    LOG("async locator mulcast:");
                    for (auto& lo : infos[i].reader_proxy_async_locator_entry.multicast) {
                        LOG("    " << lo);
                    }
                }
                LOG("------------------------------------------------");
            }
        } else {
            LOG("lookup proxy empty.");
        }
    } catch (...) {
        LOG("Usage: proxy <domain> <guid> <timeout>");
    }
}

static void remoteConfig(const std::vector<std::string>& params) {
    auto print_help_remote_config = []() {
        LOG("config <domain> <pid> logLevel:0/1/2/4/8 [host name:default using local host name] [timeout:1000]");
        LOG("config <domain> <pid> msgTraceMask:N [host name:default using local host name] [timeout]");
        LOG("config <domain> <pid> logPeriod:0 [host name:default using local host name] [timeout]");
        LOG("config <domain> <pid> addTopicFilter:topicName [host name:default using local host name] [timeout]");
        LOG("config <domain> <pid> briefOutputMode:0/1/2 [host name:default using local host name] [timeout]");
        LOG("config <domain> <pid> briefOutLimit:N [host name:default using local host name] [timeout]");
        LOG("config <domain> <pid> clearMsgTrace [host name:default using local host name] [timeout]");
    };

    if (params.size() <= 2 || !checkParam(params)) {
        print_help_remote_config();
        return;
    }
    try {
        uint32_t domain_id = atoi(params[0].c_str());
        uint32_t pid = atoi(params[1].c_str());
        std::string remote_params(params[2]);
        std::string host_name;
        uint32_t timeout = VBS_TOOL_WAIT_TIMEOUT;
        if (params.size() > 3) {
            host_name = atoi(params[3].c_str());
        } else {
            host_name = get_hostname();
        }
        if (params.size() > 4) {
            timeout = atoi(params[4].c_str());
        }
        ReturnCode_t ret = vbs::builtin::StatisticLookup::get_instance()->remoteConfig(domain_id, host_name, pid,
                                                                                       remote_params, timeout);
        if (ret == ReturnCode_t::RETCODE_OK) {
            LOG("Host name " << host_name << " pid " << pid << " remote config(" << remote_params << ") success.");
        } else {
            LOG("Host name " << host_name << " pid " << pid << " remote config(" << remote_params
                             << ") fail, err-code: " << ret());
        }
        LOG("");
    } catch (...) {
        print_help_remote_config();
    }
}

static void lookupTopicList(const std::vector<std::string>& params) {
    if (params.size() == 0 || !checkParam(params)) {
        LOG("Usage: topic-list <domain> [with participant info]");
        return;
    }
    try {
        bool with_participant_info = false;
        uint32_t domain_id = atoi(params[0].c_str());
        if (params.size() > 1) {
            with_participant_info = atoi(params[1].c_str());
        }

        std::vector<evbs::ertps::rtps::ParticipantDiscoveryInfo> infos =
            vbs::builtin::StatisticLookup::get_instance()->lookupParticipantsByDomain(domain_id, VBS_TOOL_WAIT_TIMEOUT);
        if (infos.size() == 0) {
            LOG("topic empty.");
            return;
        }

        // 初始化各列的最大宽度
        size_t nameWidth = 0;
        size_t versionWidth = 0;
        size_t startTimeWidth = 0;
        size_t hostNameWidth = 0;
        size_t kindWidth = 0;
        size_t topicNameWidth = 0;
        size_t typeWidth = 0;
        size_t guidWidth = 60;

        std::map<GUID_t, builtin::StatisticWrtierReaderInfos> detailInfos;
        LOG("===================Topic Dump===================");
        // 计算各列的最大宽度
        for (const auto& info : infos) {
            if (!info.info.isAlive ||
                info.status != evbs::ertps::rtps::ParticipantDiscoveryInfo::DISCOVERY_STATUS::DISCOVERED_PARTICIPANT) {
                continue;
            }
            nameWidth = std::max(nameWidth, info.info.m_participantName.size());
            std::string versionStr = std::to_string(info.info.m_evbsRemoteVersion[0]) + "." +
                                     std::to_string(info.info.m_evbsRemoteVersion[1]) + "." +
                                     std::to_string(info.info.m_evbsRemoteVersion[2]) + "." +
                                     std::to_string(info.info.m_evbsRemoteVersion[3]);
            versionWidth = std::max(versionWidth, versionStr.length());
            std::string startTimeStr = time_convert(info.info.m_startTime.to_ns());
            startTimeWidth = std::max(startTimeWidth, startTimeStr.length());
            hostNameWidth = std::max(hostNameWidth, info.info.m_hostName.size());

            detailInfos[info.info.m_guid] = vbs::builtin::StatisticLookup::get_instance()->lookupParticipantDetailInfo(
                domain_id, info.info.m_guid, VBS_TOOL_WAIT_TIMEOUT);
            for (const auto& detailInfo : detailInfos[info.info.m_guid]) {
                std::string kindStr =
                    (detailInfo.type == builtin::StatisticEntityType::STATICTIC_ENTITY_WRITER ? "Writer" : "Reader");
                kindWidth = std::max(kindWidth, kindStr.length());
                topicNameWidth = std::max(topicNameWidth, detailInfo.topic_name.length());
                typeWidth = std::max(typeWidth, detailInfo.type_name.length());
            }
        }

        if (!with_participant_info) {
            // 输出Topic信息表头
            LOG(std::left << std::setw(kindWidth + 2) << "Kind" << std::setw(topicNameWidth + 2) << "Topic Name"
                          << std::setw(typeWidth + 2) << "Type" << std::setw(guidWidth + 2) << "GUID");
        }

        // 输出participant信息
        for (const auto& info : infos) {
            if (!info.info.isAlive ||
                info.status != evbs::ertps::rtps::ParticipantDiscoveryInfo::DISCOVERY_STATUS::DISCOVERED_PARTICIPANT) {
                continue;
            }
            std::string versionStr = std::to_string(info.info.m_evbsRemoteVersion[0]) + "." +
                                     std::to_string(info.info.m_evbsRemoteVersion[1]) + "." +
                                     std::to_string(info.info.m_evbsRemoteVersion[2]) + "." +
                                     std::to_string(info.info.m_evbsRemoteVersion[3]);
            std::string startTimeStr = time_convert(info.info.m_startTime.to_ns());

            // 输出participant信息表头
            if (with_participant_info) {
                LOG(std::left << std::setw(nameWidth + 2) << "Participant" << std::setw(versionWidth + 2) << "Version"
                              << std::setw(startTimeWidth + 2) << "Start Time" << std::setw(hostNameWidth + 2)
                              << "Host Name");

                LOG(std::left << std::setw(nameWidth + 2) << info.info.m_participantName << std::setw(versionWidth + 2)
                              << versionStr << std::setw(startTimeWidth + 2) << startTimeStr
                              << std::setw(hostNameWidth + 2) << info.info.m_hostName);
            }
            if (with_participant_info) {
                LOG("");
            }
            if (!detailInfos[info.info.m_guid].empty()) {
                if (with_participant_info) {
                    // 输出Topic信息表头
                    LOG(std::left << std::setw(kindWidth + 2) << "Kind" << std::setw(topicNameWidth + 2) << "Topic Name"
                                  << std::setw(typeWidth + 2) << "Type" << std::setw(guidWidth + 2) << "GUID");
                }

                // 输出Topic信息
                for (const auto& detailInfo : detailInfos[info.info.m_guid]) {
                    std::string kindStr =
                        (detailInfo.type == builtin::StatisticEntityType::STATICTIC_ENTITY_WRITER ? "Writer"
                                                                                                  : "Reader");
                    LOG(std::left << std::setw(kindWidth + 2) << kindStr << std::setw(topicNameWidth + 2)
                                  << detailInfo.topic_name << std::setw(typeWidth + 2) << detailInfo.type_name
                                  << detailInfo.guid);
                }
            }
            if (with_participant_info) {
                LOG("------------------------------------------------");
            }
        }
        if (!with_participant_info) {
            LOG("------------------------------------------------");
        }
    } catch (...) {
        LOG("Usage: topic-list <domain> [with participant info]");
    }
}

std::map<std::string, std::function<void(const std::vector<std::string>&)>> commands = {
    {"part", lookupParticipant},
    {"part-detail", lookupParticipantDetail},
    {"topic", lookupTopic},
    {"match", lookupMatchInfo},
    {"send", lookupSendInfo},
    {"recv", lookupRecvInfo},
    {"qos", lookupQosInfo},
    {"msgbrief", lookupMessageBriefInfo},
    {"proxy", lookupProxyInfo},
    {"config", remoteConfig},
    {"topic-list", lookupTopicList},
#ifndef TOOL_ANDROID
    {"typediscovery", typeDiscoveryCli},
    {"ping", pingCli},
    {"record", recordCli},
    {"replay", replayCli},
    {"spy", spyCli},
#endif
};

static void print_help() {
    LOG("Usage:");
    LOG("----cmd-----------parameters----------------------------------");
    LOG("    part          <domain>        [pid:-1][detail:0]");
    LOG("    part-detail   <domain>        <guid>        ");
    LOG("    match         <domain>        <guid>        ");
    LOG("    send          <domain>        <guid>        ");
    LOG("    recv          <domain>        <guid>        ");
    LOG("    qos           <domain>        <guid>        ");
    LOG("    msgbrief      <domain>        <pid> <topic> ");
    LOG("    proxy         <domain>        <guid>        ");
    LOG("    topic         <domain>        <topic_name>  ");
    LOG("    topic-list    <domain>        [with participant info]");
    LOG("    config        <domain>        <pid> "
        "[logPeriod:0|addTopicFilter:topicName|briefOutputMode:0|briefOutLimit:N|logLevel:N]");
    LOG("    typediscovery <params>");
#ifndef TOOL_ANDROID
    LOG("    ping          <params>");
    LOG("    record        <params>");
    LOG("    replay        <params>");
    LOG("    spy           <params>");
#endif
    LOG("    exit");
    LOG("<params>: cmd -h|-help|?|--h|--help");
    LOG("--------------------------------------------------------------");
}

static int pre_config(int argc, char** argv) {
    for (int32_t i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-whitelist") == 0) {
            std::string whitelist = argv[i + 1];
            if (!whitelist.empty()) {
                vbs::DomainParticipantQosInner pqos = vbs::builtin::StatisticLookup::get_instance()->get_qos();
                auto descriptor = std::make_shared<vbs::transport::UDPv4TransportDescriptor>();
                vbsutil::xmlparser::WhiteListAddr w_addr = {whitelist, 65535};
                descriptor->interfaceWhiteList.emplace_back(w_addr);
                pqos.transport().user_transports.push_back(descriptor);
                vbs::builtin::StatisticLookup::get_instance()->set_qos(pqos);
                LOG("whilelist:" << whitelist);
            }
            i++;
        } else if (strcmp(argv[i], "-logLevel") == 0) {
            uint32_t loglevel = std::atoi(argv[i + 1]);
            vbsutil::elog::Log::SetVerbosity(loglevel);
            LOG("log level:" << loglevel);
            i++;
        } else if (strcmp(argv[i], "-mode") == 0) {
            std::string mode = argv[i + 1];
            if (mode == "superclient") {
                i++;
                std::string tmep = argv[i + 1];
                std::vector<std::string> ips = parseIPs(tmep);

                vbs::DomainParticipantQosInner pqos = vbs::builtin::StatisticLookup::get_instance()->get_qos();
                std::shared_ptr<vbs::transport::TransportDescriptorInterface> descriptor;
                auto descriptor_tmp = std::make_shared<vbs::transport::UDPv4TransportDescriptor>();
                descriptor = descriptor_tmp;
                // Add descriptor
                pqos.transport().user_transports.push_back(descriptor);
                // Set participant as DS SUPER_CLIENT
                pqos.wire_protocol().builtin.discovery_config.discoveryProtocol =
                    evbs::ertps::rtps::DiscoveryProtocol::SUPER_CLIENT;
                pqos.transport().use_builtin_transports = false;

                // Add remote SERVER to SUPER_CLIENT's list of SERVERs
                for (std::string ip : ips) {
                    std::cout << "ip:" << ip << std::endl;
                    evbs::ertps::rtps::Locator_t connection_locator;
                    connection_locator.kind = LOCATOR_KIND_UDPv4;
                    evbs::ertps::rtps::IPLocator::setIPv4(connection_locator, ip);
                    connection_locator.port = 16166;
                    pqos.wire_protocol().builtin.discovery_config.m_DiscoveryServers.push_back(connection_locator);
                }
                vbs::builtin::StatisticLookup::get_instance()->set_qos(pqos);
            } else {  // simple
                vbs::DomainParticipantQosInner pqos = vbs::builtin::StatisticLookup::get_instance()->get_qos();
                pqos.wire_protocol().builtin.discovery_config.discoveryProtocol =
                    evbs::ertps::rtps::DiscoveryProtocol::SIMPLE;
                vbs::builtin::StatisticLookup::get_instance()->set_qos(pqos);
            }
            LOG("mode:" << mode);
            i++;
        } else {
            LOG("unsupported param " << argv[i]);
            LOG("Example:");
            LOG("vbs_cli -whitelist 127.0.0.1 -logLevel 2");
            LOG("Usage:");
            LOG("-whitelist set whitelist for participant.");
            LOG("-logLevel  set log level. 0:error 2:warn 4:info 8:debug");
            LOG("-mode superclient [ip1,ip2] / simple");
#ifndef TOOL_ANDROID
            LOG("vbs_cli membuf_cli  <params>");
            LOG("vbs_cli shmmq_cli <params>");
#endif
            return -1;
        }
    }
    return 2;
}

int main(int argc, char** argv) {
    vbsutil::elog::Log::SetVerbosity(vbsutil::elog::Log::Warning);
    std::atomic<bool> running(true);

    int ret = pre_config(argc, argv);
    if (ret != 2) {
        return ret;
    }

    std::string cmd;
    std::string currentInput;
    while (running) {
        currentInput = "";
        historyIndex = inputHistory.size();
        handleInputKeys(currentInput);

        std::vector<std::string> params;
        std::regex re(R"((\"[^\"]*\"|\S+))");
        auto words_begin = std::sregex_iterator(currentInput.begin(), currentInput.end(), re);
        auto words_end = std::sregex_iterator();

        for (auto it = words_begin; it != words_end; ++it) {
            std::string match = (*it).str();
            if (!cmd.empty()) {
                if (match.front() == '"' && match.back() == '"') {
                    match = match.substr(1, match.size() - 2);  // 去掉引号
                }
                params.push_back(match);
            } else {
                cmd = match;
            }
        }

        auto it = commands.find(cmd);
        if (it != commands.end()) {
            it->second(params);  // 执行找到的函数
        } else if (cmd == "exit" || cmd == "q") {
            vbs::builtin::StatisticLookup::get_instance()->release();
            break;
        } else {
            print_help();
        }
        cmd.clear();
    }
    running = false;
    return 0;
}
