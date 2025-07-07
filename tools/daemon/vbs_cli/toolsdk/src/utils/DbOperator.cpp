#include <iostream>

#include <elog/Log.hpp>
#include "utils/DbOperator.hpp"
#include <sqlite3.h>

namespace vbstoolsdk {

static constexpr const char* TABLE_METADATA_CREATE =
    "CREATE TABLE IF NOT EXISTS metadata("
    "topic_id INTEGER PRIMARY KEY AUTOINCREMENT,topic_name VARCHAR NOT NULL,"
    "data_type VARCHAR NOT NULL ,topic_type_name VARCHAR NOT NULL ,topic_idl VARCHAR ,"
    "topic_idl_xml VARCHAR NOT NULL ,start_timestamp INTEGER ,end_timestamp INTEGER ,"
    "description VARCHAR, domain_id INTEGER)";
static constexpr const char* TABLE_RAWDATA_CREATE =
    "CREATE TABLE IF NOT EXISTS rawdata(id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "topic_id INTEGER NOT NULL,raw_timestamp INTEGER ,local_timestamp INTEGER ,raw_data BLOB ,"
    "sequence_number INTEGER ,raw_json VARCHAR ,"
    "FOREIGN KEY (topic_id) REFERENCES metadata(topic_id) ON DELETE RESTRICT ON UPDATE CASCADE)";
static constexpr const char* METADATA_INSERT_SQL =
    "INSERT INTO metadata"
    "(topic_id, topic_name, data_type, topic_type_name,"
    " topic_idl, topic_idl_xml, description, domain_id) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

static constexpr const char* RAWDATA_INSERT_SQL =
    "INSERT INTO "
    "rawdata(topic_id, raw_timestamp, local_timestamp, raw_data, sequence_number)"
    "VALUES (?, ?, ?, ?, ?)";

static constexpr const char* METADATA_SELECT_SQL =
    "SELECT "
    "topic_id, topic_name, data_type, topic_type_name, topic_idl, topic_idl_xml, domain_id "
    "FROM metadata";

DbOperator::DbOperator(const std::string& full_path, bool need_create)
    : if_new_tables_(need_create), db_path_(full_path), db_(nullptr) {
    init_db_();
}

DbOperator::~DbOperator() {
    if (db_) {
        vbs_sqlite3_close((vbsSqlite3*)db_);
        db_ = nullptr;
    }
}

void DbOperator::db_exec_cmd_(const std::string& command) {
    if (!db_) {
        logError_("VBSTOOLSDK", "empty db or db not init !!!");
        return;
    }

    char* errmsg = nullptr;
    int rc = vbs_sqlite3_exec((vbsSqlite3*)db_, command.c_str(), nullptr, nullptr, &errmsg);

    if (rc != VBS_SQLITE_OK) {
        logError_("VBSTOOLSDK", "SQL error: " + std::string(errmsg));
        vbs_sqlite3_free(errmsg);
    }
}

void DbOperator::init_db_() {
    if (db_)
        return;
    vbsSqlite3* db_ptr = nullptr;
    int rc =
        vbs_sqlite3_open_v2(db_path_.c_str(), &db_ptr, VBS_SQLITE_OPEN_READWRITE | VBS_SQLITE_OPEN_CREATE, nullptr);

    if (rc != VBS_SQLITE_OK) {
        logError_("VBSTOOLSDK", "open db failed, error msg : " << vbs_sqlite3_errmsg(db_ptr));
        vbs_sqlite3_close(db_ptr);
        return;
    }
    db_ = db_ptr;

    if (if_new_tables_) {
        db_exec_cmd_(TABLE_METADATA_CREATE);
        db_exec_cmd_(TABLE_RAWDATA_CREATE);
    }
}

#if 0
bool DbOperator::if_table_exist_in_db_(const std::string& table_name)
{
    const char* query = "SELECT name FROM VBS_SQLITE_master WHERE type='table' AND name=?";
    vbs_sqlite3_stmt* stmt = nullptr;
    bool exists = false;

    if (vbs_sqlite3_prepare_v2((vbsSqlite3*)db_, query, -1, &stmt, nullptr) != VBS_SQLITE_OK) {
        logError_("VBSTOOLSDK", "Failed to prepare statement: "
                                    + std::string(vbs_sqlite3_errmsg((vbsSqlite3*)db_)));
        return false;
    }

    vbs_sqlite3_bind_text(stmt, 1, table_name.c_str(), -1, VBS_SQLITE_TRANSIENT);

    if (vbs_sqlite3_step(stmt) == VBS_SQLITE_ROW) {
        exists = true;
    }

    vbs_sqlite3_finalize(stmt);
    return exists;
}
#endif

bool DbOperator::add_metadata(DbMetaDataInfo& info) {
    vbs_sqlite3_stmt* stmt = nullptr;
    bool success = true;

    char* errmsg = nullptr;
    if (vbs_sqlite3_exec((vbsSqlite3*)db_, "BEGIN TRANSACTION;", nullptr, nullptr, &errmsg) != VBS_SQLITE_OK) {
        logError_("VBSTOOLSDK", "Error starting transaction: " + std::string(errmsg));
        vbs_sqlite3_free(errmsg);
        return false;
    }

    if (vbs_sqlite3_prepare_v2((vbsSqlite3*)db_, METADATA_INSERT_SQL, -1, &stmt, nullptr) != VBS_SQLITE_OK) {
        logError_("VBSTOOLSDK", "Failed to prepare statement: " + std::string(vbs_sqlite3_errmsg((vbsSqlite3*)db_)));
        return false;
    }

    vbs_sqlite3_bind_int(stmt, 1, info.topic_id);
    vbs_sqlite3_bind_text(stmt, 2, info.topic_name.c_str(), -1, VBS_SQLITE_TRANSIENT);
    vbs_sqlite3_bind_text(stmt, 3, info.data_type.c_str(), -1, VBS_SQLITE_TRANSIENT);
    vbs_sqlite3_bind_text(stmt, 4, info.topic_type_name.c_str(), -1, VBS_SQLITE_TRANSIENT);
    vbs_sqlite3_bind_text(stmt, 5, info.topic_idl.c_str(), -1, VBS_SQLITE_TRANSIENT);
    vbs_sqlite3_bind_text(stmt, 6, info.topic_idl_xml.c_str(), -1, VBS_SQLITE_TRANSIENT);
    vbs_sqlite3_bind_text(stmt, 7, info.description.c_str(), -1, VBS_SQLITE_TRANSIENT);
    vbs_sqlite3_bind_int(stmt, 8, info.domain_id);

    if (vbs_sqlite3_step(stmt) != VBS_SQLITE_DONE) {
        logError_("VBSTOOLSDK", "Execution failed: " + std::string(vbs_sqlite3_errmsg((vbsSqlite3*)db_)));
        success = false;
    }

    vbs_sqlite3_finalize(stmt);

    if (success) {
        if (vbs_sqlite3_exec((vbsSqlite3*)db_, "COMMIT;", nullptr, nullptr, &errmsg) != VBS_SQLITE_OK) {
            logError_("VBSTOOLSDK", "Error committing transaction: " + std::string(errmsg));
            vbs_sqlite3_free(errmsg);
            success = false;
        }
    } else {
        if (vbs_sqlite3_exec((vbsSqlite3*)db_, "ROLLBACK;", nullptr, nullptr, &errmsg) != VBS_SQLITE_OK) {
            logError_("VBSTOOLSDK", "Error rolling back transaction: " + std::string(errmsg));
            vbs_sqlite3_free(errmsg);
        }
    }

    return success;
}

bool DbOperator::add_metadata(std::vector<DbMetaDataInfo>& info_vec) {
    vbs_sqlite3_stmt* stmt = nullptr;
    char* errmsg = nullptr;
    bool success = true;

    if (vbs_sqlite3_exec((vbsSqlite3*)db_, "BEGIN TRANSACTION;", nullptr, nullptr, &errmsg) != VBS_SQLITE_OK) {
        std::cerr << "Failed to start transaction: " << errmsg << std::endl;
        vbs_sqlite3_free(errmsg);
        return false;
    }

    if (vbs_sqlite3_prepare_v2((vbsSqlite3*)db_, METADATA_INSERT_SQL, -1, &stmt, nullptr) != VBS_SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << vbs_sqlite3_errmsg((vbsSqlite3*)db_) << std::endl;
        return false;
    }

    for (const auto& info : info_vec) {
        vbs_sqlite3_bind_int(stmt, 1, info.topic_id);
        vbs_sqlite3_bind_text(stmt, 2, info.topic_name.c_str(), -1, VBS_SQLITE_TRANSIENT);
        vbs_sqlite3_bind_text(stmt, 3, info.data_type.c_str(), -1, VBS_SQLITE_TRANSIENT);
        vbs_sqlite3_bind_text(stmt, 4, info.topic_type_name.c_str(), -1, VBS_SQLITE_TRANSIENT);
        vbs_sqlite3_bind_text(stmt, 5, info.topic_idl.c_str(), -1, VBS_SQLITE_TRANSIENT);
        vbs_sqlite3_bind_text(stmt, 6, info.topic_idl_xml.c_str(), -1, VBS_SQLITE_TRANSIENT);
        vbs_sqlite3_bind_text(stmt, 7, info.description.c_str(), -1, VBS_SQLITE_TRANSIENT);
        vbs_sqlite3_bind_int(stmt, 8, info.domain_id);

        if (vbs_sqlite3_step(stmt) != VBS_SQLITE_DONE) {
            std::cerr << "Execution failed: " << vbs_sqlite3_errmsg((vbsSqlite3*)db_) << std::endl;
            success = false;
            break;
        }

        vbs_sqlite3_reset(stmt);
    }

    vbs_sqlite3_finalize(stmt);

    if (success) {
        if (vbs_sqlite3_exec((vbsSqlite3*)db_, "COMMIT;", nullptr, nullptr, &errmsg) != VBS_SQLITE_OK) {
            std::cerr << "Failed to commit transaction: " << errmsg << std::endl;
            vbs_sqlite3_free(errmsg);
            return false;
        }
    } else {
        if (vbs_sqlite3_exec((vbsSqlite3*)db_, "ROLLBACK;", nullptr, nullptr, &errmsg) != VBS_SQLITE_OK) {
            std::cerr << "Failed to rollback transaction: " << errmsg << std::endl;
            vbs_sqlite3_free(errmsg);
        }
    }

    return success;
}

bool DbOperator::get_metadata(std::vector<DbMetaDataInfo>& info_vec) {
    vbs_sqlite3_stmt* stmt = nullptr;
    bool success = true;

    if (vbs_sqlite3_prepare_v2((vbsSqlite3*)db_, METADATA_SELECT_SQL, -1, &stmt, nullptr) != VBS_SQLITE_OK) {
        logError_("VBSTOOLSDK", "Failed to prepare statement: " + std::string(vbs_sqlite3_errmsg((vbsSqlite3*)db_)));
        return false;
    }

    while (vbs_sqlite3_step(stmt) == VBS_SQLITE_ROW) {
        DbMetaDataInfo info;
        info.topic_id = vbs_sqlite3_column_int(stmt, 0);
        info.data_type = reinterpret_cast<const char*>(vbs_sqlite3_column_text(stmt, 2));

        if (info.data_type != "dds") {
            logError_("VBSTOOLSDK",
                      "topic id: " + std::to_string(info.topic_id) + " is not dds, is: " + info.data_type);
            continue;
        }

        info.topic_name = reinterpret_cast<const char*>(vbs_sqlite3_column_text(stmt, 1));
        info.topic_type_name = reinterpret_cast<const char*>(vbs_sqlite3_column_text(stmt, 3));
        info.topic_idl = reinterpret_cast<const char*>(vbs_sqlite3_column_text(stmt, 4));
        info.topic_idl_xml = reinterpret_cast<const char*>(vbs_sqlite3_column_text(stmt, 5));
        info.domain_id = vbs_sqlite3_column_int(stmt, 6);

        info_vec.emplace_back(std::move(info));
    }

    if (vbs_sqlite3_errcode((vbsSqlite3*)db_) != VBS_SQLITE_DONE &&
        vbs_sqlite3_errcode((vbsSqlite3*)db_) != VBS_SQLITE_OK) {
        logError_("VBSTOOLSDK", "Failed to execute query: " + std::string(vbs_sqlite3_errmsg((vbsSqlite3*)db_)));
        success = false;
    }

    vbs_sqlite3_finalize(stmt);
    return success;
}

bool DbOperator::update_metadata_timestamp(UpdateTimestampInfo& info) {
    vbs_sqlite3_stmt* stmt = nullptr;
    char* errmsg = nullptr;
    bool success = true;

    if (vbs_sqlite3_exec((vbsSqlite3*)db_, "BEGIN TRANSACTION;", nullptr, nullptr, &errmsg) != VBS_SQLITE_OK) {
        logError_("VBSTOOLSDK", "Error starting transaction: " + std::string(errmsg));
        vbs_sqlite3_free(errmsg);
        return false;
    }

    std::string cmd = "UPDATE metadata SET " + info.key + " = ? WHERE topic_name = ?";

    if (vbs_sqlite3_prepare_v2((vbsSqlite3*)db_, cmd.c_str(), -1, &stmt, nullptr) != VBS_SQLITE_OK) {
        logError_("VBSTOOLSDK", "Failed to prepare statement: " + std::string(vbs_sqlite3_errmsg((vbsSqlite3*)db_)));
        return false;
    }

    vbs_sqlite3_bind_int64(stmt, 1, info.value);
    vbs_sqlite3_bind_text(stmt, 2, info.topic_name.c_str(), -1, VBS_SQLITE_TRANSIENT);

    if (vbs_sqlite3_step(stmt) != VBS_SQLITE_DONE) {
        logError_("VBSTOOLSDK", "Execution failed: " + std::string(vbs_sqlite3_errmsg((vbsSqlite3*)db_)));
        success = false;
    }

    vbs_sqlite3_finalize(stmt);

    if (success) {
        if (vbs_sqlite3_exec((vbsSqlite3*)db_, "COMMIT;", nullptr, nullptr, &errmsg) != VBS_SQLITE_OK) {
            logError_("VBSTOOLSDK", "Error committing transaction: " + std::string(errmsg));
            vbs_sqlite3_free(errmsg);
            return false;
        }
    } else {
        if (vbs_sqlite3_exec((vbsSqlite3*)db_, "ROLLBACK;", nullptr, nullptr, &errmsg) != VBS_SQLITE_OK) {
            logError_("VBSTOOLSDK", "Error rolling back transaction: " + std::string(errmsg));
            vbs_sqlite3_free(errmsg);
        }
    }

    return success;
}

bool DbOperator::is_topic_name_existed(const std::string& topic_name, int& topic_id) {
    const char* cmd = "SELECT topic_id FROM metadata WHERE topic_name = ?";
    vbs_sqlite3_stmt* stmt = nullptr;
    bool exists = false;

    if (vbs_sqlite3_prepare_v2((vbsSqlite3*)db_, cmd, -1, &stmt, nullptr) != VBS_SQLITE_OK) {
        logError_("VBSTOOLSDK", "Failed to prepare statement: " + std::string(vbs_sqlite3_errmsg((vbsSqlite3*)db_)));
        return false;
    }

    if (vbs_sqlite3_bind_text(stmt, 1, topic_name.c_str(), -1, VBS_SQLITE_TRANSIENT) != VBS_SQLITE_OK) {
        logError_("VBSTOOLSDK", "Failed to bind parameter: " + std::string(vbs_sqlite3_errmsg((vbsSqlite3*)db_)));
        vbs_sqlite3_finalize(stmt);
        return false;
    }

    if (vbs_sqlite3_step(stmt) == VBS_SQLITE_ROW) {
        topic_id = vbs_sqlite3_column_int(stmt, 0);
        exists = true;
    }

    vbs_sqlite3_finalize(stmt);

    return exists;
}

bool DbOperator::add_rawdata(DbRawDataInfo& info) {

    vbs_sqlite3_stmt* stmt = nullptr;
    bool success = true;

    char* errmsg = nullptr;
    if (vbs_sqlite3_exec((vbsSqlite3*)db_, "BEGIN TRANSACTION;", nullptr, nullptr, &errmsg) != VBS_SQLITE_OK) {
        logError_("VBSTOOLSDK", "Error starting transaction: " + std::string(errmsg));
        vbs_sqlite3_free(errmsg);
        return false;
    }

    if (vbs_sqlite3_prepare_v2((vbsSqlite3*)db_, RAWDATA_INSERT_SQL, -1, &stmt, nullptr) != VBS_SQLITE_OK) {
        logError_("VBSTOOLSDK", "Failed to prepare statement: " + std::string(vbs_sqlite3_errmsg((vbsSqlite3*)db_)));
        return false;
    }

    vbs_sqlite3_bind_int(stmt, 1, info.topic_id);
    vbs_sqlite3_bind_int64(stmt, 2, info.raw_timestamp);
    vbs_sqlite3_bind_int64(stmt, 3, info.local_timestamp);
    vbs_sqlite3_bind_blob(stmt, 4, info.raw_data.data(), (int)info.raw_data.size(), VBS_SQLITE_TRANSIENT);
    vbs_sqlite3_bind_int(stmt, 5, info.sequence_number);

    if (vbs_sqlite3_step(stmt) != VBS_SQLITE_DONE) {
        logError_("VBSTOOLSDK", "Execution failed: " + std::string(vbs_sqlite3_errmsg((vbsSqlite3*)db_)));
        success = false;
    }

    vbs_sqlite3_finalize(stmt);

    if (success) {
        if (vbs_sqlite3_exec((vbsSqlite3*)db_, "COMMIT;", nullptr, nullptr, &errmsg) != VBS_SQLITE_OK) {
            logError_("VBSTOOLSDK", "Error committing transaction: " + std::string(errmsg));
            vbs_sqlite3_free(errmsg);
            success = false;
        }
    } else {
        if (vbs_sqlite3_exec((vbsSqlite3*)db_, "ROLLBACK;", nullptr, nullptr, &errmsg) != VBS_SQLITE_OK) {
            logError_("VBSTOOLSDK", "Error rolling back transaction: " + std::string(errmsg));
            vbs_sqlite3_free(errmsg);
        }
    }

    return success;
}

bool DbOperator::add_rawdata(std::vector<DbRawDataInfo>& info_vec) {
    vbs_sqlite3_stmt* stmt = nullptr;
    char* errmsg = nullptr;
    bool success = true;

    if (vbs_sqlite3_exec((vbsSqlite3*)db_, "BEGIN TRANSACTION;", nullptr, nullptr, &errmsg) != VBS_SQLITE_OK) {
        std::cerr << "Failed to start transaction: " << errmsg << std::endl;
        vbs_sqlite3_free(errmsg);
        return false;
    }

    if (vbs_sqlite3_prepare_v2((vbsSqlite3*)db_, RAWDATA_INSERT_SQL, -1, &stmt, nullptr) != VBS_SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << vbs_sqlite3_errmsg((vbsSqlite3*)db_) << std::endl;
        return false;
    }

    for (const auto& info : info_vec) {
        vbs_sqlite3_bind_int(stmt, 1, info.topic_id);
        vbs_sqlite3_bind_int64(stmt, 2, info.raw_timestamp);
        vbs_sqlite3_bind_int64(stmt, 3, info.local_timestamp);
        vbs_sqlite3_bind_blob(stmt, 4, info.raw_data.data(), (int)info.raw_data.size(), VBS_SQLITE_TRANSIENT);
        vbs_sqlite3_bind_int(stmt, 5, info.sequence_number);

        if (vbs_sqlite3_step(stmt) != VBS_SQLITE_DONE) {
            std::cerr << "Execution failed: " << vbs_sqlite3_errmsg((vbsSqlite3*)db_) << std::endl;
            success = false;
            break;
        }

        vbs_sqlite3_reset(stmt);
    }

    vbs_sqlite3_finalize(stmt);

    if (success) {
        if (vbs_sqlite3_exec((vbsSqlite3*)db_, "COMMIT;", nullptr, nullptr, &errmsg) != VBS_SQLITE_OK) {
            std::cerr << "Failed to commit transaction: " << errmsg << std::endl;
            vbs_sqlite3_free(errmsg);
            return false;
        }
    } else {
        if (vbs_sqlite3_exec((vbsSqlite3*)db_, "ROLLBACK;", nullptr, nullptr, &errmsg) != VBS_SQLITE_OK) {
            std::cerr << "Failed to rollback transaction: " << errmsg << std::endl;
            vbs_sqlite3_free(errmsg);
        }
    }

    return success;
}

int DbOperator::get_rawdata(std::vector<DbRawDataInfo>& info_vec, int begin, int end) {
    static const char* cmd = "SELECT topic_id, raw_data, raw_timestamp FROM rawdata LIMIT ?, ?";
    vbs_sqlite3_stmt* stmt = nullptr;
    int count = 0;

    if (vbs_sqlite3_prepare_v2((vbsSqlite3*)db_, cmd, -1, &stmt, nullptr) != VBS_SQLITE_OK) {
        logError_("VBSTOOLSDK", "Failed to prepare statement: " + std::string(vbs_sqlite3_errmsg((vbsSqlite3*)db_)));
        return 0;
    }

    vbs_sqlite3_bind_int(stmt, 1, begin);
    vbs_sqlite3_bind_int(stmt, 2, end - begin);

    while (vbs_sqlite3_step(stmt) == VBS_SQLITE_ROW) {
        DbRawDataInfo info;
        info.topic_id = vbs_sqlite3_column_int(stmt, 0);

        const void* buffer = vbs_sqlite3_column_blob(stmt, 1);
        int data_length = vbs_sqlite3_column_bytes(stmt, 1);

        info.raw_data.resize(data_length);
        memcpy(info.raw_data.data(), buffer, data_length);

        info.raw_timestamp = vbs_sqlite3_column_int64(stmt, 2);

        info_vec.emplace_back(std::move(info));
        count++;
    }

    if (vbs_sqlite3_errcode((vbsSqlite3*)db_) != VBS_SQLITE_DONE &&
        vbs_sqlite3_errcode((vbsSqlite3*)db_) != VBS_SQLITE_OK) {
        logError_("VBSTOOLSDK", "Failed to execute query: " + std::string(vbs_sqlite3_errmsg((vbsSqlite3*)db_)));
    }

    vbs_sqlite3_finalize(stmt);
    return count;
}

int DbOperator::get_rawdata_count() {
    static const char* cmd = "SELECT count(*) FROM rawdata";
    vbs_sqlite3_stmt* stmt = nullptr;
    int count = 0;

    if (vbs_sqlite3_prepare_v2((vbsSqlite3*)db_, cmd, -1, &stmt, nullptr) != VBS_SQLITE_OK) {
        logError_("VBSTOOLSDK", "Failed to prepare statement: " + std::string(vbs_sqlite3_errmsg((vbsSqlite3*)db_)));
        return 0;
    }

    if (vbs_sqlite3_step(stmt) == VBS_SQLITE_ROW) {
        count = vbs_sqlite3_column_int(stmt, 0);
    } else {
        logError_("VBSTOOLSDK", "Failed to execute query: " + std::string(vbs_sqlite3_errmsg((vbsSqlite3*)db_)));
    }

    vbs_sqlite3_finalize(stmt);
    return count;
}

}  // namespace vbstoolsdk