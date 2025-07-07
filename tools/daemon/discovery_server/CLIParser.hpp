// Copyright 2024 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------
// Modification history:
// feature: add multi-domain support for discovery server
// feature: support agent between domain
// feature: (discovery_server): add platform config and optimize defaults
// ------------------------------------------------------------------

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdio>
#include <libgen.h>
#include <string>
#include <vector>

#include <elog/Log.hpp>
#include <erpc/utils/RpcLog.h>
#include <deps/common/LocatorList.hpp>
#include <ertps/types/TypesBase.h>
#include <elog/FileConsumer.hpp>
#include <filesystem>
#include <deps/common/Locator.h>

#include "Helpers.hpp"

#ifndef VBS_CPP_DISCOVERY_SERVER__CLIPARSER_HPP
#define VBS_CPP_DISCOVERY_SERVER__CLIPARSER_HPP

namespace discovery_server {

using vbs::common::types::DOMAIN_ID_MAX;
using vbs::common::types::DomainId_t;
using vbsutil::elog::Log;
using vbsutil::xmlparser::Locator_t;
using vbsutil::xmlparser::LocatorList_t;

const size_t MAX_FILENAME_LENGTH = 255;

inline bool hasInvalidCharacters(const std::string& name) {
    std::regex invalid_chars(R"([^\w\-.])");
    return std::regex_search(name, invalid_chars);
}

inline bool isValidFileName(const std::string& name) {
    return !name.empty() && name.size() <= MAX_FILENAME_LENGTH && !hasInvalidCharacters(name);
}

inline bool storeStringInAddress(const std::string& input, Locator_t& locator) {
    if (input.size() >= LOCATOR_ADDRESS_MAX_LEN) {
        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                  "Error: Input string is too long to be stored in the address array.");
        return false;
    }

    std::strncpy(reinterpret_cast<char*>(locator.address), input.c_str(), LOCATOR_ADDRESS_MAX_LEN - 1);
    locator.address[LOCATOR_ADDRESS_MAX_LEN - 1] = '\0';
    return true;
}

inline std::string addressToString(const Locator_t& locator) {
    return std::string(reinterpret_cast<const char*>(locator.address));
}

inline void setup_application_logging(const std::string& log_dir, const std::string& node_name) {
    // vbsutil::elog::Log::ClearConsumers();
    vbsutil::elog::Log::RegisterConsumer(std::unique_ptr<vbsutil::elog::LogConsumer>(
        new vbsutil::elog::FileConsumer(log_dir, true, 1024, true, node_name)));
}

inline bool ensure_log_directory(const std::string& log_directory, std::string& error_message) {
    if (access(log_directory.c_str(), F_OK) == 0) {
        if (access(log_directory.c_str(), W_OK) == 0) {
            return true;
        } else {
            error_message = "Log directory '" + log_directory + "' is not writable.";
            return false;
        }
    } else {
#ifndef ANDROID
        std::error_code ec;
        if (!std::filesystem::create_directories(log_directory, ec)) {
            error_message = "Failed to create log directory: " + log_directory + ", error: " + ec.message();
            return false;
        } else {
            return true;
        }
#else
        error_message = "Failed to create log directory: " + log_directory;
        return false;
#endif
    }
}

inline bool contains_slash(const std::string& str) {
    return str.find('/') != std::string::npos;
}

// Recursively create paths
inline bool createDirectoryRecursive(const std::string& path) {
    // Temporarily clear umask
    mode_t old_mask = umask(0);
    size_t pos = 0;
    while ((pos = path.find('/', pos + 1)) != std::string::npos) {
        std::string subPath = path.substr(0, pos);
        if (subPath.empty()) continue;
        struct stat st;
        if (stat(subPath.c_str(), &st) == -1) {
            if (mkdir(subPath.c_str(), 0777) == -1 && errno != EEXIST) {
                return false;
            }
        } else if (!S_ISDIR(st.st_mode)) {
            errno = ENOTDIR;
            return false;
        }
    }
    struct stat st;
    if (stat(path.c_str(), &st) == -1) {
        if (mkdir(path.c_str(), 0777) == -1 && errno != EEXIST) {
            return false;
        }
    }
    // Restore original umask
    umask(old_mask);
    return true;
}

class CLIParser {
 public:
    CLIParser() = delete;

    //! Entity kind enumeration
    enum class EntityKind : uint8_t { CLIENT_PUBLISHER, CLIENT_SUBSCRIBER, SERVER, UNDEFINED };
    //! Mode kind enumeration
    enum class ServerModeKind : uint8_t { ALL_SERVER, RPC_SERVER, DDS_SERVER, AGENT_SERVER, UNDEFINED };

    friend std::ostream& operator<<(std::ostream& os, const ServerModeKind mode) {
        switch (mode) {
            case ServerModeKind::ALL_SERVER:
                os << "ALL_SERVER";
                break;
            case ServerModeKind::RPC_SERVER:
                os << "RPC_SERVER";
                break;
            case ServerModeKind::DDS_SERVER:
                os << "DDS_SERVER";
                break;
            case ServerModeKind::AGENT_SERVER:
                os << "AGENT_SERVER";
                break;
            case ServerModeKind::UNDEFINED:
                os << "UNDEFINED";
                break;
            default:
                os << "UNKNOWN";
        }
        return os;
    }

    //! Clients common configuration
    struct client_config {
        // uint16_t connection_port{16166};
        // std::string connection_address{"127.0.0.1"};
        LocatorList_t connection_list;
        // std::string listening_address{"127.0.0.1"};
        LocatorList_t listening_list;
        uint16_t domain_id {0};
        int32_t announcement {30};
        int32_t duration {120};
        bool is_print_table {false};
    };

    //! Configuration options for both publisher and subscriber clients
    struct pubsub_config : public client_config {
        bool reliable {false};
        bool transient_local {false};
        uint16_t samples {0};
        std::string topic_name {"discovery_server_topic"};
    };

    //! Publisher client configuration structure
    struct client_publisher_config : public pubsub_config {
        // TransportKind transport_kind{TransportKind::UDPv4};
        uint16_t interval {100};
    };

    //! Subscriber client configuration structure
    struct client_subscriber_config : public pubsub_config {
        // TransportKind transport_kind{TransportKind::UDPv4};
    };

    //! Server configuration structure
    //! A server can, in turn, act as a client
    struct server_config : public client_config {
        CLIParser::ServerModeKind server_mode = CLIParser::ServerModeKind::DDS_SERVER;
        std::string whitelist;
        std::string agent_config_path;
        bool use_agent_composite {false};
        bool is_also_client {false};
        bool is_backup {false};
        bool agent_inter_host {false};
        std::string platform;  // Add platform field for new option
        bool is_compatible {false};
        uint16_t timeout {0};
        uint16_t agent_domain {0};
        std::vector<uint16_t> domain_list;  // 新增 domain_list 成员
        std::string log_file_path;
    };

    //! Configuration structure for the example
    struct ds_config {
        CLIParser::EntityKind entity = CLIParser::EntityKind::UNDEFINED;
        client_publisher_config pub_config;
        client_subscriber_config sub_config;
        server_config srv_config;

        friend std::ostream& operator<<(std::ostream& os, const ds_config& config) {
            os << "Entity: " << parse_entity_kind(config.entity) << std::endl;
            os << "Common options:" << std::endl;

            if (config.entity != CLIParser::EntityKind::SERVER ||
                (config.entity == CLIParser::EntityKind::SERVER && config.srv_config.is_also_client)) {
                os << "Client options:" << std::endl;
                uint32_t connect_num = 0;
                for (auto it_loc = config.pub_config.connection_list.begin();
                     it_loc != config.pub_config.connection_list.end(); ++it_loc) {
                    connect_num++;
                    os << "  Connection locator " << connect_num << std::endl;
                    os << "    Connection Transport: " << static_cast<int>(it_loc->kind) << std::endl;
                    os << "    Connection address: " << it_loc->address << std::endl;
                    os << "    Connection port: " << it_loc->port << std::endl;
                }
            }

            if (config.entity == CLIParser::EntityKind::CLIENT_PUBLISHER) {
                os << "Publisher options:" << std::endl;
                os << "  Topic name: " << config.pub_config.topic_name << std::endl;
                os << "  Samples: " << config.pub_config.samples << std::endl;
                os << "  Interval: " << config.pub_config.interval << std::endl;
            } else if (config.entity == CLIParser::EntityKind::CLIENT_SUBSCRIBER) {
                os << "Subscriber options:" << std::endl;
                os << "  Topic name: " << config.sub_config.topic_name << std::endl;
                os << "  Samples: " << config.sub_config.samples << std::endl;
            } else if (config.entity == CLIParser::EntityKind::SERVER) {
                os << "Server options:" << std::endl;
                os << "  Server mode: " << config.srv_config.server_mode << "(0:all, 1:rpc, 2:dds, 3:agent)"
                   << std::endl;
                uint32_t listen_num = 0;
                for (auto it_loc = config.srv_config.listening_list.begin();
                     it_loc != config.srv_config.listening_list.end(); ++it_loc) {
                    listen_num++;
                    os << "  Listening locator " << listen_num << std::endl;
                    os << "    Listening Transport: " << static_cast<int>(it_loc->kind) << std::endl;
                    os << "    Listening address: " << it_loc->address << std::endl;
                    os << "    Listening port: " << it_loc->port << std::endl;
                }
                os << "  Timeout: " << config.srv_config.timeout << std::endl;
            }

            return os;
        }
    };

    /**
     * @brief Print usage help message and exit with the given return code
     *
     * @param return_code return code to exit with
     *
     * @warning This method finishes the execution of the program with the input return code
     */
    static void print_help(uint8_t return_code) {
        std::cout << "Usage: ./DServer <entity> [options]" << std::endl;
        std::cout << "" << std::endl;
        std::cout << "Entities:" << std::endl;
        std::cout << "  publisher                            Run a client publisher entity." << std::endl;
        std::cout << "  subscriber                           Run a client subscriber entity." << std::endl;
        std::cout << "  server                               Run a server entity." << std::endl;
        std::cout << "" << std::endl;
        std::cout << "  --announcement <num>                 set lease announcement(Seconds)." << std::endl;
        std::cout << "  --duration <num>                     set lease duration(Seconds)." << std::endl;
        std::cout << "  --loglevel <num>                     set log level(0:Debug, 1:Info, 2:Warning, 3:Error)."
                  << std::endl;
        std::cout << "  -h,       --help                     Print this help message." << std::endl;
        std::cout << "Client options (common to Publisher, Subscriber and Server acting as Client):" << std::endl;
        std::cout << "  -c <str>, --connection-locator <str> Address of the Server to connect to" << std::endl;
        std::cout << "                                       (Default address: udpv4:127.0.0.1:16166, this option can "
                     "be configured multiple times.)."
                  << std::endl;
        std::cout << "                                       [udpv4:A.B.C.D:PORT|uds:file] " << std::endl;
        std::cout << "" << std::endl;
        std::cout << "Publisher options:" << std::endl;
        std::cout << "  -t <str>, --topic <str>              Topic name" << std::endl;
        std::cout << "                                       (Default: discovery_server_topic)." << std::endl;
        std::cout << "  -r, --reliable                       Set Reliability QoS as reliable" << std::endl;
        std::cout << "                                       (Default: best effort)" << std::endl;
        std::cout << "      --transient-local                Set Durability QoS as transient local" << std::endl;
        std::cout << "                                       (Default: volatile)" << std::endl;
        std::cout << "  -s <num>, --samples <num>            Number of samples to send " << std::endl;
        std::cout << "                                       (Default: 0 => infinite samples)." << std::endl;
        std::cout << "  -i <num>, --interval <num>           Time between samples in milliseconds" << std::endl;
        std::cout << "                                       (Default: 100)." << std::endl;
        std::cout << "  --domain-id <num>                    Domain Id" << std::endl;
        std::cout << "                                       (Default: 0)." << std::endl;
        std::cout << "" << std::endl;
        std::cout << "Subscriber options:" << std::endl;
        std::cout << "  -t <str>, --topic <str>              Topic name" << std::endl;
        std::cout << "                                       (Default: discovery_server_topic)." << std::endl;
        std::cout << "  -s <num>, --samples <num>            Number of samples to receive" << std::endl;
        std::cout << "                                       (Default: 0 => infinite samples)." << std::endl;
        std::cout << "  -r, --reliable                       Set Reliability QoS as reliable" << std::endl;
        std::cout << "                                       (Default: best effort)" << std::endl;
        std::cout << "      --transient-local                Set Durability QoS as transient local" << std::endl;
        std::cout << "                                       (Default: volatile)" << std::endl;
        std::cout << "  --domain-id <num>                    Domain Id" << std::endl;
        std::cout << "                                       (Default: 0)." << std::endl;
        std::cout << "" << std::endl;
        std::cout << "Server options:" << std::endl;
        std::cout << "  --mode <str>               Mode of server" << std::endl;
        std::cout << "                                       (Default: all)." << std::endl;
        std::cout << "                                       [all|rpc|dds|agent]" << std::endl;
        std::cout << "  --listening-locator <str>  Server listening address" << std::endl;
        std::cout << "                                       (Default address: udpv4:127.0.0.1:16166, this option can "
                     "be configured multiple times.)."
                  << std::endl;
        std::cout << "                                       [udpv4:A.B.C.D:PORT|uds:file] " << std::endl;
        std::cout << "  --timeout <num>            Number of seconds before finish" << std::endl;
        std::cout << "  --table-item               Print the table item information offline" << std::endl;
        std::cout << "  --compatible               Support compatible, can discover simple endpoint." << std::endl;
        std::cout << "                                       the process (Default: 0 = till ^C)." << std::endl;
        std::cout << "  --backup                   Backup enable." << std::endl;
        std::cout << "  --whitelist                Agent participant whitelist. e.g.: 127.0.0.1" << std::endl;
        std::cout << "  --agent-composite          Enable composite agent." << std::endl;
        std::cout << "  --agent-inter-host         Enable agent between different host. " << std::endl;
        std::cout << "  --domain-list <num1,num2,...>  Set multiple domain IDs for server" << std::endl;
        std::cout << "                                       (e.g.: --domain-list 1,2,3)." << std::endl;
        std::cout << "  --agent-config-path        Set agent config file. " << std::endl;
        std::cout << "  --log-path <path>            Set log directory" << std::endl;
        std::cout << "                                       (e.g.: --log-path /your/custom/log/path)." << std::endl;
        std::exit(return_code);
    }

    // Here you would parse "udpv4:127.0.0.1:16166"
    // into locator_t
    static void parse_locator(const std::string& value, Locator_t& locator) {
        size_t colon_pos1 = value.find(':');
        size_t colon_pos2 = value.rfind(':');
        std::string transport, address;
        uint16_t port = 16166;

        locator.port = 16166;
        if (colon_pos1 == std::string::npos) {
            elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                      "Invalid format for --listening-locator value: Transport and Address must be specified.");
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            print_help(EXIT_FAILURE);
        }

        transport = value.substr(0, colon_pos1);
        if (transport == "udpv4") {
            locator.kind = LOCATOR_KIND_UDPv4;
            if (colon_pos2 != std::string::npos && colon_pos1 != colon_pos2) {
                address = value.substr(colon_pos1 + 1, colon_pos2 - colon_pos1 - 1);
                // Check if DNS is required
                if (!is_ip(address)) {
                    address = get_ip_from_dns(address, LOCATOR_KIND_UDPv4);
                }
                if (!evbs::ertps::rtps::IPLocator::setIPv4(locator, address)) {
                    elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                              "Unkown address argument: " + address);
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    print_help(EXIT_FAILURE);
                }
                try {
                    port = std::stoi(value.substr(colon_pos2 + 1));
                    if (port < std::numeric_limits<uint16_t>::min() || port > std::numeric_limits<uint16_t>::max()) {
                        throw std::out_of_range("listening-port argument " + value.substr(colon_pos2 + 1) +
                                                " out of range [0, 65535].");
                    } else {
                        evbs::ertps::rtps::IPLocator::setPhysicalPort(locator, port);
                    }
                } catch (const std::invalid_argument& e) {
                    elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                              "invalid listening-port argument " + value.substr(colon_pos2 + 1) + ": " +
                                  std::string(e.what()));
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    print_help(EXIT_FAILURE);
                } catch (const std::out_of_range& e) {
                    elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, std::string(e.what()));
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    print_help(EXIT_FAILURE);
                }
            } else {
                address = value.substr(colon_pos1 + 1);
                // Check if DNS is required
                if (!is_ip(address)) {
                    address = get_ip_from_dns(address, LOCATOR_KIND_UDPv4);
                }
                if (!evbs::ertps::rtps::IPLocator::setIPv4(locator, address)) {
                    elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                              "Unkown address argument: " + address);
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    print_help(EXIT_FAILURE);
                }
            }
        } else if (transport == "uds") {
            locator.kind = LOCATOR_KIND_UDS;
            address = value.substr(colon_pos1 + 1);

            if (!storeStringInAddress(address, locator)) {
                elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, "Unkown address argument: " + address);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                print_help(EXIT_FAILURE);
            }
            // elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
            //         " Under Development ......");
            // print_help(EXIT_SUCCESS);
        } else {
            elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, "Unkown transport argument: " + transport);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            print_help(EXIT_FAILURE);
        }

        elogInfo(DISCOVERY_SERVICE, "  Transport Type: " << transport);
        elogInfo(DISCOVERY_SERVICE, "  Address: " << address);
        elogInfo(DISCOVERY_SERVICE, "  Port: " << port);
    }
    /**
     * @brief Parse the command line options and return the configuration_config object
     *
     * @param argc number of arguments
     * @param argv array of arguments
     * @return configuration_config object with the parsed options
     *
     * @warning This method finishes the execution of the program if the input arguments are invalid
     */
    static ds_config parse_cli_options(int argc, char* argv[]) {
        ds_config config;
        config.entity = CLIParser::EntityKind::SERVER;  // Set default entity as SERVER

        bool listening_address_was_set = false;
        bool connection_address_was_set = false;
        int start_index = 1;  // Default start from first argument
        bool listening_have_uds_path = false;

        if (argc >= 2) {
            std::string first_argument = argv[1];
            if (first_argument == "publisher") {
                config.entity = CLIParser::EntityKind::CLIENT_PUBLISHER;
                start_index = 2;  // Skip entity argument
            } else if (first_argument == "subscriber") {
                config.entity = CLIParser::EntityKind::CLIENT_SUBSCRIBER;
                start_index = 2;  // Skip entity argument
            } else if (first_argument == "-h" || first_argument == "--help") {
                print_help(EXIT_SUCCESS);
            } else if (first_argument == "server") {
                start_index = 2;                               // Skip entity argument
            } else if (first_argument.substr(0, 2) != "--") {  // Not an option
                elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                          "parsing entity argument " + first_argument);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                print_help(EXIT_FAILURE);
            }
        }

        for (int i = start_index; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "-h" || arg == "--help") {
                print_help(EXIT_SUCCESS);
            } else if (arg == "--mode") {
                if (config.entity == CLIParser::EntityKind::SERVER && ++i < argc) {
                    std::string value = argv[i];
                    elogInfo(DISCOVERY_SERVICE, "Server mode : " << value);
                    if (value == "all") {
                        config.srv_config.server_mode = CLIParser::ServerModeKind::ALL_SERVER;
                    } else if (value == "rpc") {
                        config.srv_config.server_mode = CLIParser::ServerModeKind::RPC_SERVER;
                    } else if (value == "dds") {
                        config.srv_config.server_mode = CLIParser::ServerModeKind::DDS_SERVER;
                    } else if (value == "agent") {
                        config.srv_config.server_mode = CLIParser::ServerModeKind::AGENT_SERVER;
                    } else {
                        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                                  "wrong server mode, only support all|rpc|dds|agent. ");
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        print_help(EXIT_FAILURE);
                    }
                }
            } else if (arg == "--announcement") {
                if (i + 1 < argc) {
                    try {
                        int input = std::stoi(argv[++i]);
                        if (input < 0 || input > std::numeric_limits<std::int32_t>::max()) {
                            throw std::out_of_range("announcement argument out of range");
                        } else {
                            config.pub_config.announcement = static_cast<int32_t>(input);
                            config.sub_config.announcement = static_cast<int32_t>(input);
                            config.srv_config.announcement = static_cast<int32_t>(input);
                        }
                    } catch (const std::out_of_range& e) {
                        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                                  "announcement argument out of range for " + arg + ": " + e.what());
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        print_help(EXIT_FAILURE);
                    }
                } else {
                    elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, "missing argument for " + arg);
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    print_help(EXIT_FAILURE);
                }
            } else if (arg == "--duration") {
                if (i + 1 < argc) {
                    try {
                        int input = std::stoi(argv[++i]);
                        if (input < 0 || input > std::numeric_limits<std::int32_t>::max()) {
                            throw std::out_of_range("duration argument out of range");
                        } else {
                            config.pub_config.duration = static_cast<int32_t>(input);
                            config.sub_config.duration = static_cast<int32_t>(input);
                            config.srv_config.duration = static_cast<int32_t>(input);
                        }
                    } catch (const std::out_of_range& e) {
                        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                                  "duration argument out of range for " + arg + ": " + e.what());
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        print_help(EXIT_FAILURE);
                    }
                } else {
                    elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, "missing argument for " + arg);
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    print_help(EXIT_FAILURE);
                }
            } else if (arg == "--loglevel") {
                if (i + 1 < argc) {
                    int loglevel = std::stoi(argv[++i]);
                    if (loglevel == 0) {
                        Log::SetVerbosity(Log::Debug);
                        ::vbs::rpc::RpcLog::SetRpcLogLevel(::vbs::rpc::RpcLogKind::Debug);
                    } else if (loglevel == 1) {
                        Log::SetVerbosity(Log::Info);
                        ::vbs::rpc::RpcLog::SetRpcLogLevel(::vbs::rpc::RpcLogKind::Info);
                    } else if (loglevel == 2) {
                        Log::SetVerbosity(Log::Warning);
                        ::vbs::rpc::RpcLog::SetRpcLogLevel(::vbs::rpc::RpcLogKind::Warning);
                    } else if (loglevel == 3) {
                        Log::SetVerbosity(Log::Error);
                        ::vbs::rpc::RpcLog::SetRpcLogLevel(::vbs::rpc::RpcLogKind::Error);
                    } else {
                        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                                  "Log level is error level=" << loglevel << "(0:Debug, 1:Info, 2:Warning, 3:Error)");
                    }
                }
            } else if (arg == "--table-item") {
                config.srv_config.is_print_table = true;
            } else if (arg == "--compatible") {
                config.srv_config.is_compatible = true;
            }
            // Client options
            else if (arg == "--connection-locator") {
                if (++i < argc) {
                    std::string value = argv[i];
                    // Process the locator value
                    elogInfo(DISCOVERY_SERVICE, "Connecting to locator: " << value);
                    Locator_t connection_locator;
                    parse_locator(value, connection_locator);

                    config.pub_config.connection_list.push_back(connection_locator);
                    config.sub_config.connection_list.push_back(connection_locator);
                    config.srv_config.connection_list.push_back(connection_locator);
                    if (config.entity == CLIParser::EntityKind::SERVER) {
                        config.srv_config.is_also_client = true;
                    }
                    connection_address_was_set = true;
                }
            }
            // PubSub options
            else if (arg == "-t" || arg == "--topic") {
                if (++i < argc) {
                    if (config.entity == CLIParser::EntityKind::CLIENT_PUBLISHER ||
                        config.entity == CLIParser::EntityKind::CLIENT_SUBSCRIBER) {
                        config.pub_config.topic_name = argv[i];
                        config.sub_config.topic_name = argv[i];
                    } else {
                        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                                  "wrong or missing entity for --topic argument: only available for publisher and "
                                  "subscriber");
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        print_help(EXIT_FAILURE);
                    }
                }
            } else if (arg == "-s" || arg == "--samples") {
                if (i + 1 < argc) {
                    try {
                        int input = std::stoi(argv[++i]);
                        if (input < std::numeric_limits<std::uint16_t>::min() ||
                            input > std::numeric_limits<std::uint16_t>::max()) {
                            throw std::out_of_range("samples argument out of range");
                        } else {
                            if (config.entity == CLIParser::EntityKind::CLIENT_PUBLISHER ||
                                config.entity == CLIParser::EntityKind::CLIENT_SUBSCRIBER) {
                                config.pub_config.samples = static_cast<uint16_t>(input);
                                config.sub_config.samples = static_cast<uint16_t>(input);
                            } else {
                                elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                                          "entity error or not specified for --samples argument");
                                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                                print_help(EXIT_FAILURE);
                            }
                        }
                    } catch (const std::invalid_argument& e) {
                        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                                  "invalid sample argument for " + arg + ": " + e.what());
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        print_help(EXIT_FAILURE);
                    } catch (const std::out_of_range& e) {
                        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                                  "samples argument out of range for " + arg + ": " + e.what());
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        print_help(EXIT_FAILURE);
                    }
                } else {
                    elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, "missing argument for " + arg);
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    print_help(EXIT_FAILURE);
                }
            } else if (arg == "--reliable") {
                config.pub_config.reliable = true;
                config.sub_config.reliable = true;
            } else if (arg == "--transient-local") {
                config.pub_config.transient_local = true;
                config.sub_config.transient_local = true;
            }
            // Publisher options
            else if (arg == "-i" || arg == "--interval") {
                if (config.entity == CLIParser::EntityKind::CLIENT_PUBLISHER) {
                    if (++i < argc) {
                        try {
                            int input = std::stoi(argv[i]);
                            if (input < std::numeric_limits<std::uint16_t>::min() ||
                                input > std::numeric_limits<std::uint16_t>::max()) {
                                throw std::out_of_range("interval argument out of range");
                            } else {
                                config.pub_config.interval = static_cast<uint16_t>(input);
                            }
                        } catch (const std::invalid_argument& e) {
                            elogError(
                                DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                                "invalid interval argument " + std::string(argv[i]) + ": " + std::string(e.what()));
                            std::this_thread::sleep_for(std::chrono::milliseconds(200));
                            print_help(EXIT_FAILURE);
                        } catch (const std::out_of_range& e) {
                            elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, std::string(e.what()));
                            std::this_thread::sleep_for(std::chrono::milliseconds(200));
                            print_help(EXIT_FAILURE);
                        }
                    } else {
                        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, "parsing interval argument");
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        print_help(EXIT_FAILURE);
                    }
                } else {
                    elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                              "interval argument is only valid for client publisher entity");
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    print_help(EXIT_FAILURE);
                }
            }
            // Server options
            else if (arg == "--listening-locator") {
                if (++i < argc) {
                    std::string value = argv[i];
                    // Process the locator value
                    std::cout << "Listenning to locator: " << value << std::endl;

                    // if (config.entity != CLIParser::EntityKind::SERVER) {
                    //     elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                    //               "listening address  argument is only valid for server entity");
                    //     std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    //     print_help(EXIT_FAILURE);
                    // }

                    // Process the locator value
                    elogInfo(DISCOVERY_SERVICE, "Listening to locator: " << value);
                    Locator_t listening_locator;
                    parse_locator(value, listening_locator);

                    config.srv_config.listening_list.push_back(listening_locator);
                    config.pub_config.listening_list.push_back(listening_locator);
                    config.sub_config.listening_list.push_back(listening_locator);
                    elogInfo(DISCOVERY_SERVICE, "Listening to locatorlist: " << config.srv_config.listening_list);
                    listening_address_was_set = true;
                    if (listening_locator.kind == LOCATOR_KIND_UDS && contains_slash(value)) {
                        listening_have_uds_path = true;
                    }
                }
            } else if (arg == "--backup") {
                if (config.entity == CLIParser::EntityKind::SERVER) {
                    config.srv_config.is_backup = true;
                } else {
                    elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                              "--backup argument is only valid for server entity");
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    print_help(EXIT_FAILURE);
                }
            } else if (arg == "--timeout") {
                if (++i < argc) {
                    if (config.entity == CLIParser::EntityKind::SERVER) {
                        try {
                            int input = std::stoi(argv[i]);
                            if (input < std::numeric_limits<uint16_t>::min() ||
                                input > std::numeric_limits<uint16_t>::max()) {
                                throw std::out_of_range("timeout argument " + std::string(argv[i]) +
                                                        " out of range [0, 65535].");
                            } else {
                                config.srv_config.timeout = static_cast<uint16_t>(input);
                            }
                        } catch (const std::invalid_argument& e) {
                            elogError(
                                DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                                "invalid timeout argument " + std::string(argv[i]) + ": " + std::string(e.what()));
                            std::this_thread::sleep_for(std::chrono::milliseconds(200));
                            print_help(EXIT_FAILURE);
                        } catch (const std::out_of_range& e) {
                            elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, std::string(e.what()));
                            std::this_thread::sleep_for(std::chrono::milliseconds(200));
                            print_help(EXIT_FAILURE);
                        }
                    } else {
                        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                                  "--listening-port argument is only valid for server entity");
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        print_help(EXIT_FAILURE);
                    }
                } else {
                    elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, "parsing port argument");
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    print_help(EXIT_FAILURE);
                }
            } else if (arg == "--domain-id") {
                if (++i < argc) {
                    if (config.entity == CLIParser::EntityKind::CLIENT_PUBLISHER ||
                        config.entity == CLIParser::EntityKind::CLIENT_SUBSCRIBER) {
                        try {
                            int input = std::stoi(argv[i]);
                            if (input < std::numeric_limits<uint16_t>::min() ||
                                input > static_cast<int>(DOMAIN_ID_MAX)) {
                                throw std::out_of_range("domain-id argument " + std::string(argv[i]) +
                                                        " out of range.");
                            } else {
                                config.pub_config.domain_id = static_cast<uint16_t>(input);
                                config.sub_config.domain_id = static_cast<uint16_t>(input);
                            }
                        } catch (const std::invalid_argument& e) {
                            elogError(
                                DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                                "invalid doamin-id argument " + std::string(argv[i]) + ": " + std::string(e.what()));
                            std::this_thread::sleep_for(std::chrono::milliseconds(200));
                            print_help(EXIT_FAILURE);
                        } catch (const std::out_of_range& e) {
                            elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, std::string(e.what()));
                            std::this_thread::sleep_for(std::chrono::milliseconds(200));
                            print_help(EXIT_FAILURE);
                        }
                    } else {
                        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                                  "--listening-port argument is only valid for server entity");
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        print_help(EXIT_FAILURE);
                    }
                } else {
                    elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, "parsing port argument");
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    print_help(EXIT_FAILURE);
                }
            } else if (arg == "--whitelist") {
                if (++i < argc) {
                    std::string value = argv[i];
                    config.srv_config.whitelist = value;
                    elogInfo(DISCOVERY_SERVICE, "Add whitelist: " << value);
                }
            } else if (arg == "--agent-inter-host") {
                config.srv_config.agent_inter_host = true;
                elogInfo(DISCOVERY_SERVICE, "Enable agent communication between inter host.");
            } else if (arg == "--agent-composite") {
                config.srv_config.use_agent_composite = true;
                elogInfo(DISCOVERY_SERVICE, "Enable composite agent.");
            } else if (arg == "--agent-config-path") {
                if (++i < argc) {
                    std::string value = argv[i];
                    config.srv_config.agent_config_path = value;
                    elogInfo(DISCOVERY_SERVICE, "Agent config file: " << value);
                }
            } else if (arg == "--domain-list") {
                if (++i < argc) {
                    std::string domains = argv[i];
                    std::stringstream ss(domains);
                    std::string domain;

                    while (std::getline(ss, domain, ',')) {
                        try {
                            int input = std::stoi(domain);
                            if (input < std::numeric_limits<uint16_t>::min() ||
                                input > static_cast<int>(DOMAIN_ID_MAX)) {
                                throw std::out_of_range("domain-id " + domain + " out of range.");
                            }
                            config.srv_config.domain_list.push_back(static_cast<uint16_t>(input));
                        } catch (const std::invalid_argument& e) {
                            elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                                      "invalid domain-id " + domain + ": " + std::string(e.what()));
                            std::this_thread::sleep_for(std::chrono::milliseconds(200));
                            print_help(EXIT_FAILURE);
                        } catch (const std::out_of_range& e) {
                            elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, std::string(e.what()));
                            std::this_thread::sleep_for(std::chrono::milliseconds(200));
                            print_help(EXIT_FAILURE);
                        }
                    }

                    if (config.srv_config.domain_list.empty()) {
                        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                                  "No valid domain IDs provided in domain-list");
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        print_help(EXIT_FAILURE);
                    }

                    elogInfo(DISCOVERY_SERVICE,
                             "Domain list configured with " << config.srv_config.domain_list.size() << " domains");
                } else {
                    elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                              "missing argument for --domain-list");
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    print_help(EXIT_FAILURE);
                }
            } else if (arg == "--log-path") {
                if (++i < argc) {
                    std::string full_log_path_input = argv[i];
                    std::string log_directory;
#ifndef ANDROID
                    try {
                        std::filesystem::path p(full_log_path_input);
                        log_directory = p.parent_path().string();
                        if (log_directory.empty()) {
                            log_directory = ".";
                        }
                    } catch (const std::exception& e) {
                        elogError(
                            DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                            "Invalid path provided for --log-path: " << full_log_path_input << ", error: " << e.what());
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    }

                    std::string error_message;
                    if (ensure_log_directory(log_directory, error_message)) {
                        config.srv_config.log_file_path = full_log_path_input;
                        setup_application_logging(config.srv_config.log_file_path, "Other");
                        elogInfo(DISCOVERY_SERVICE, "Log file path configured: " << config.srv_config.log_file_path
                                                                                 << ", directory '" << log_directory
                                                                                 << "' is ready.");
                    } else {
                        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, error_message);
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    }
#else
                    elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, "cannot support --log-path.");
#endif
                } else {
                    elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                              "missing argument for --log-path (expected full file path)");
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    print_help(EXIT_FAILURE);
                }
            } else {
                elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, "unknown option " + arg);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                print_help(EXIT_FAILURE);
            }
        }
#if !defined(_WIN32)
        if (!listening_have_uds_path) {
            struct stat st = {0};
            if (stat("/dev/socket/vbs", &st) == -1) {
                if (!createDirectoryRecursive("/dev/socket/vbs")) {
                    elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                              "cannot create /dev/socket/vbs, please check the permission");
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    print_help(EXIT_FAILURE);
                }
            } else if (!S_ISDIR(st.st_mode)) {
                elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, "/dev/socket/vbs is not a directory");
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                print_help(EXIT_FAILURE);
            }
            // check the permission of /dev/socket/vbs
            if (access("/dev/socket/vbs", W_OK | X_OK) != 0) {
                elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER,
                          "cannot access /dev/socket/vbs, please check the permission");
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                print_help(EXIT_FAILURE);
            }
        }
#else
        if (!listening_have_uds_path) {
            elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, "UDS is not supported on Windows");
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            print_help(EXIT_FAILURE);
        }
#endif
        // set default values if user did not specified ones
        if (config.entity == CLIParser::EntityKind::SERVER && !listening_address_was_set) {
            elogInfo(DISCOVERY_SERVICE, "Defalut listening to locator: uds:/dev/socket/vbs/dserver");
            Locator_t listening_locator;
            parse_locator("uds:dserver", listening_locator);

            config.srv_config.listening_list.push_back(listening_locator);
        }

        if (!connection_address_was_set) {
            elogInfo(DISCOVERY_SERVICE, "Defalut connecting to uds locator: uds:/dev/socket/vbs/dserver");
        }

        return config;
    }

    /**
     * @brief Parse the signal number into the signal name
     *
     * @param signum signal number
     * @return std::string signal name
     */
    static std::string parse_signal(const int& signum) {
        switch (signum) {
            case SIGINT:
                return "SIGINT";
            case SIGTERM:
                return "SIGTERM";
#ifndef _WIN32
            case SIGQUIT:
                return "SIGQUIT";
            case SIGHUP:
                return "SIGHUP";
#endif  // _WIN32
            default:
                return "UNKNOWN SIGNAL";
        }
    }

    /**
     * @brief Parse the entity kind into std::string
     *
     * @param entity entity kind
     * @return std::string entity kind
     */
    static std::string parse_entity_kind(const EntityKind& entity) {
        switch (entity) {
            case EntityKind::CLIENT_PUBLISHER:
                return "Client Publisher";
            case EntityKind::CLIENT_SUBSCRIBER:
                return "Client Subscriber";
            case EntityKind::SERVER:
                return "Discovery Server";
            case EntityKind::UNDEFINED:
            default:
                return "Undefined entity";
        }
    }
};

}  // namespace discovery_server

#endif  // VBS_CPP_DISCOVERY_SERVER__CLIPARSER_HPP
