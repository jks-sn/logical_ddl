CREATE SCHEMA IF NOT EXISTS logical_ddl;

CREATE TABLE IF NOT EXISTS logical_ddl.ddl_commands (
    id SERIAL PRIMARY KEY,
    command_type TEXT NOT NULL,
    command_tag TEXT NOT NULL,
    schema_name TEXT,
    relation_name TEXT,
    command_text TEXT NOT NULL,
    executed_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS logical_ddl.command_status (
    id SERIAL PRIMARY KEY,
    command_id INT NOT NULL,
    status TEXT NOT NULL,
    result TEXT,
    error_message TEXT,
    executed_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
);

CREATE PUBLICATION logical_ddl_status_pub FOR TABLE logical_ddl.command_status;

CREATE SUBSCRIPTION logical_ddl_sub 
CONNECTION 'host=localhost port=5432 dbname=postgres'
PUBLICATION logical_ddl_pub;

