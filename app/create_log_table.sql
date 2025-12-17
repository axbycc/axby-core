create table if not exists log (

topic varchar,

-- header
sender_process_id ubigint,
sender_sequence_id ubigint,
sender_process_time_us ubigint,
protocol_version usmallint,
message_version usmallint,
flags usmallint,

this_process_time_us ubigint,
message_id ubigint,

-- payload
frames blob, --cbor list of bytes

);

create table if not exists metadata (

this_process_id ubigint,
creation_process_time_us ubigint,
creation_unix_time_ms ubigint,

);

