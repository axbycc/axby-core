#pragma once

#include <duckdb.h>
#include <optional>

#include <cstdint>
#include <string_view>
#include <type_traits>

#include "debug/log.h"
#include "seq/seq.h"

// Specialization of to_span for duckdb_string
inline std::span<const std::byte> to_span(const duckdb_string_t& duck_str) {
    const auto length = duckdb_string_t_length(duck_str);
    const auto data =
        (std::byte*)duckdb_string_t_data((duckdb_string_t*)(&duck_str));
    return std::span<const std::byte>{data, length};
}

// Specialization of value_type for duckdb_string
template <>
struct value_type<duckdb_string_t> {
    using type = const std::byte;
};

inline void DUCKDB_CHECKED_QUERY(duckdb_state state, duckdb_result* result) {
    if (state != DuckDBSuccess) {
        LOG(FATAL) << duckdb_result_error(result);
    }
}

inline void DUCKDB_CHECKED_PREPARE(duckdb_state state,
                                   duckdb_prepared_statement& prepare) {
    if (state != DuckDBSuccess) {
        LOG(FATAL) << duckdb_prepare_error(prepare);
    }
}
inline void DUCKDB_CHECK(duckdb_state state) { CHECK(state == DuckDBSuccess); }

// an RAII for the DuckDB C API bundling a db and connection see
// DuckDbConnection for standalone connection
class DuckDbContext {
   public:
    // call init() to create a valid db_ and conn_
    void init(std::string_view db_path);

    ~DuckDbContext();

    bool initted_ = false;
    duckdb_database db_;
    duckdb_connection conn_;
};

void check_duckdb_appender_error(duckdb_appender appender);

template <typename T>
struct duckdb_vector_helper {
    // the C API provides provides 2 functions for pulling a column
    // (or vector as it is called in duckdb terminology) from a
    // duckdb_data_chunk. those are
    //
    // 1. duckdb_data_chunk_get_vector: ptr to the data
    // 2. duckdb_data_chunk_get_validity: ptr to a buffer about nullity of each row
    //
    // This struct just combines them in the constructor to make it
    // easy to them in tandem.
    //
    // Also adds some conveience methods.

    idx_t size;
    duckdb_vector vector;
    T* data;
    uint64_t* validity;
    duckdb_type type;

    duckdb_type get_type() const {
        // must temporarily allocate a "duckdb_logical_type" object
        duckdb_logical_type ltype = duckdb_vector_get_column_type(vector);
        duckdb_type type = duckdb_get_type_id(ltype);
        duckdb_destroy_logical_type(&ltype);
        return type;
    }

    bool is_valid(idx_t row) const {
        return duckdb_validity_row_is_valid(validity, row);
    }

    T& row(idx_t r) {
        CHECK(r < size);
        return data[r];
    }

    duckdb_vector_helper(duckdb_data_chunk chunk, idx_t idx) {
        size = duckdb_data_chunk_get_size(chunk);
        vector = duckdb_data_chunk_get_vector(chunk, idx);
        data = (T*)duckdb_vector_get_data(vector);
        validity = duckdb_vector_get_validity(vector);
    }
};

// RAII helper for duckdb_result, takes ownership of the result
class DuckDbResult {
   public:
    // takes ownership of the result
    DuckDbResult(duckdb_result result);
    DuckDbResult(duckdb_connection con, const char* sql);

    // calls duckdb_destroy_result
    ~DuckDbResult();

    DuckDbResult(const DuckDbResult&) = delete;
    DuckDbResult& operator=(const DuckDbResult&) = delete;

    // call fetch_chunk() until is_done(). each call destroys the
    // previous chunk. returns !is_done() so you can do
    // while(fetch_chunk())
    bool fetch_chunk();
    bool is_done();

    template <typename T>
    duckdb_vector_helper<T> get_column(idx_t idx) {
        CHECK(fetched_any_) << "haven't called fetch_chunk";
        CHECK(!is_done()) << "chunk is empty. no more results";
        return duckdb_vector_helper<T>{chunk_, idx};
    };

    // get num rows of the current chunk
    idx_t get_num_rows();

   private:
    bool fetched_any_ = false;
    duckdb_result result_;
    duckdb_data_chunk chunk_;
};

// RAII Helper
class DuckDbConnection {
public:
    DuckDbConnection(duckdb_database db);
    ~DuckDbConnection();

    DuckDbConnection(const DuckDbConnection&) = delete;
    DuckDbConnection& operator=(const DuckDbConnection&) = delete;

    // convenience decay to underlying C type for easy interfacing
    // with the C API
    operator duckdb_connection&() { return con_; }

    duckdb_connection con_;
};

class DuckDbPreparedStatement {
   public:
    DuckDbPreparedStatement(duckdb_connection con, const char* sql);
    ~DuckDbPreparedStatement();

    DuckDbPreparedStatement(const DuckDbPreparedStatement&) = delete;
    DuckDbPreparedStatement& operator=(const DuckDbPreparedStatement&) = delete;

    void bind_param_uint64(const char* param_name, uint64_t value);
    void bind_param_string(const char* param_name, std::string_view value);
    void execute();

    // must call execute() first before accessing result
    DuckDbResult& result();

   private:
    duckdb_prepared_statement prepared_statement_;

    // filled by execute()
    std::optional<DuckDbResult> result_;
};

inline std::string_view duckdb_string_to_string_view(
    const duckdb_string_t& duck_str) {
    return {duckdb_string_t_data((duckdb_string_t*)(&duck_str)),
            duckdb_string_t_length(duck_str)};
}

inline std::string duckdb_string_to_string(const duckdb_string_t& duck_str) {
    return std::string(duckdb_string_to_string_view(duck_str));
}

inline void duckdb_string_to_buff(const duckdb_string_t& duck_str,
                                  axby::Seq<std::byte> buff) {
    seq_copy(duck_str, buff);
}
