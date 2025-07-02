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

#ifndef DSFDDS_GUID_H
#define DSFDDS_GUID_H

#include <iomanip>
#include <cstring>
#include <cstdint>

namespace li {
namespace dsfdds {

#define USER_PARTICIPANT_KIND 0x01
#define USER_WRITER_KIND 0x03
#define BUILTIN_WRITER_KIND 0xc3
#define USER_READER_KIND 0x04
#define BUILTIN_READER_KIND 0xc4
#define BUILTIN_ENTITY_ID_START_INDEX 3000

enum EntityType : int8_t {
    ENTITY_WRITER,
    ENTITY_READER,
    PARTICIPANT,
    BUILTIN_WRITER,
    BUILTIN_READER,
    ENTITY_TYPE_UNKNOW,
};
struct EntityId_s {
    uint8_t value[4];
};

struct GuidPrefix_s {
    uint8_t value[12];
    static bool GenerateGuidPrefix(uint16_t participant_id, uint8_t did, GuidPrefix_s* guidPrefixS);
};

struct GUID_s {
    GuidPrefix_s guidPrefix;
    EntityId_s entityId;

    EntityType get_entity_type_from_guid() const {
        uint8_t e_type_;
        static_cast<void>(memcpy(&e_type_, entityId.value + 3, 1U));
        if (e_type_ == USER_READER_KIND) {
            return ENTITY_READER;
        } else if (e_type_ == USER_WRITER_KIND) {
            return ENTITY_WRITER;
        } else if (e_type_ == BUILTIN_READER_KIND) {
            return BUILTIN_READER;
        } else if (e_type_ == BUILTIN_WRITER_KIND) {
            return BUILTIN_WRITER;
        } else if (e_type_ == USER_PARTICIPANT_KIND) {
            return PARTICIPANT;
        } else {
            return ENTITY_TYPE_UNKNOW;
        }
    }

    uint8_t get_domain_id_from_guid() const {
        uint8_t did_ = 0U;
        static_cast<void>(memcpy(&did_, guidPrefix.value + 5, 1U));
        return did_;
    }

    uint32_t get_pid_from_guid() const {
        uint32_t pid_ = 0U;
        static_cast<void>(memcpy(&pid_, guidPrefix.value + 6, 4U));
        return pid_;
    }

    uint16_t get_participant_id_from_guid() const {
        uint16_t participant_id_ = 0U;
        static_cast<void>(memcpy(&participant_id_, guidPrefix.value + 10, 2U));
        return participant_id_;
    }

    uint32_t get_entity_id_from_guid() const {
        uint32_t entity_id_ = 0U;
        static_cast<void>(memcpy(&entity_id_, entityId.value, 3U));
        return entity_id_;
    }

    bool operator==(const GUID_s& rhs) const {
        return (std::memcmp(guidPrefix.value, rhs.guidPrefix.value, 12) == 0) &&
               (std::memcmp(entityId.value, rhs.entityId.value, 3) == 0);
    }

    bool operator!=(const GUID_s& rhs) const { return !operator==(rhs); }

    /*The less than operator of the Guid only guarantees the difference between PID, DomainId, ParticipantId,
  * and EntityId. When using a Guid as the key for a map.
  */
    bool operator<(const GUID_s& rhs) const {
        if (std::memcmp(guidPrefix.value + 5, rhs.guidPrefix.value + 5, 7) < 0) {
            return true;
        } else if (std::memcmp(guidPrefix.value + 5, rhs.guidPrefix.value + 5, 7) == 0) {
            return std::memcmp(entityId.value, rhs.entityId.value, 3) < 0;
        }
        return false;
    }
};

inline std::ostream& operator<<(std::ostream& os, const GUID_s& guid) {
    uint8_t e_type = guid.get_entity_type_from_guid();
    if (e_type == PARTICIPANT) {
        os << "Entity type: participant.";
    } else if (e_type == ENTITY_WRITER || e_type == BUILTIN_WRITER) {
        os << "Entity type: writer.";
    } else if (e_type == ENTITY_READER || e_type == BUILTIN_READER) {
        os << "Entity type: reader.";
    } else {
        os << "Entity type: type unknow!";
    }
    os << "GuidPrefix vendor_id:";
    for (int i = 0; i < 2; ++i) {
        os << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(guid.guidPrefix.value[i]);
    }
    os << ",GuidPrefix mac:";
    for (int i = 2; i < 5; ++i) {
        os << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(guid.guidPrefix.value[i]);
    }
    os << std::dec << ",pid:" << guid.get_pid_from_guid()
       << ",domain_id:" << static_cast<int>(guid.get_domain_id_from_guid())
       << ",participant_id:" << guid.get_participant_id_from_guid();
    os << ",EntityId_s:" << guid.get_entity_id_from_guid();
    return os;
}

}  // namespace dsfdds
}  // namespace li

#endif  // DSFDDS_GUID_H
