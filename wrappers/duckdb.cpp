#include "wrappers/duckdb.h"

void DuckDbContext::init(std::string_view db_path) {
    CHECK(!initted_) << "double init";

    char* open_error_string = nullptr;
    auto open_result = duckdb_open_ext(std::string(db_path).c_str(), &db_,
                                       nullptr, &open_error_string);
    if (open_result == DuckDBError) {
        LOG(FATAL) << open_error_string << " when opening " << db_path;
        duckdb_free(open_error_string);
    }

    CHECK(duckdb_connect(db_, &conn_) != DuckDBError);

    initted_ = true;
}

DuckDbContext::~DuckDbContext() {
    if (initted_) {
        duckdb_disconnect(&conn_);
        duckdb_close(&db_);
    }
}

void check_duckdb_appender_error(duckdb_appender appender) {
    const char* err = duckdb_appender_error(appender);
    LOG_IF(FATAL, err != nullptr) << err;
}

DuckDbResult::DuckDbResult(duckdb_result r) : result_(r) {};

DuckDbResult::DuckDbResult(duckdb_connection con, const char* sql) {
    DUCKDB_CHECKED_QUERY(duckdb_query(con, sql, &result_), &result_);
};

DuckDbResult::~DuckDbResult() {
    if (fetched_any_ && bool(chunk_)) {
        duckdb_destroy_data_chunk(&chunk_);
    }
    duckdb_destroy_result(&result_);
}
bool DuckDbResult::fetch_chunk() {
    if (fetched_any_) {
        // release previous chunk
        duckdb_destroy_data_chunk(&chunk_);
    }
    chunk_ = duckdb_fetch_chunk(result_);
    fetched_any_ = true;

    return !is_done();
}

bool DuckDbResult::is_done() {
    if (!fetched_any_) {
        return false;
    }
    return !bool(chunk_);
}

idx_t DuckDbResult::get_num_rows() {
    CHECK(fetched_any_);
    return duckdb_data_chunk_get_size(chunk_);
}

DuckDbPreparedStatement::DuckDbPreparedStatement(duckdb_connection con,
                                                 const char* sql) {
    DUCKDB_CHECKED_PREPARE(duckdb_prepare(con, sql, &prepared_statement_),
                           prepared_statement_);
};

DuckDbPreparedStatement::~DuckDbPreparedStatement() {
    duckdb_destroy_prepare(&prepared_statement_);
};

void DuckDbPreparedStatement::bind_param_uint64(const char* param_name,
                                                uint64_t value) {
    idx_t idx = 0;
    DUCKDB_CHECKED_PREPARE(
        duckdb_bind_parameter_index(prepared_statement_, &idx, param_name),
        prepared_statement_);
    DUCKDB_CHECKED_PREPARE(duckdb_bind_uint64(prepared_statement_, idx, value),
                           prepared_statement_);
}

void DuckDbPreparedStatement::bind_param_string(const char* param_name,
                                                std::string_view value) {
    idx_t idx = 0;
    DUCKDB_CHECKED_PREPARE(
        duckdb_bind_parameter_index(prepared_statement_, &idx, param_name),
        prepared_statement_);
    DUCKDB_CHECKED_PREPARE(
        duckdb_bind_varchar_length(prepared_statement_, idx, value.data(),
                                   value.size()),
        prepared_statement_);
}

void DuckDbPreparedStatement::execute() {
    CHECK(!result_.has_value());

    duckdb_result result;
    DUCKDB_CHECKED_QUERY(duckdb_execute_prepared(prepared_statement_, &result),
                         &result);

    result_.emplace(result);
}

DuckDbResult& DuckDbPreparedStatement::result() {
    CHECK(result_.has_value());
    return *result_;
}

DuckDbConnection::DuckDbConnection(duckdb_database db) {
    DUCKDB_CHECK(duckdb_connect(db, &con_));
}

DuckDbConnection::~DuckDbConnection() { duckdb_disconnect(&con_); }
