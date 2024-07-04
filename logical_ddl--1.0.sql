/* contrib/logical_ddl/logical_ddl--1.0.sql */

\echo Use "CREATE EXTENSION logical_ddl" to load this file. \quit

COMMENT ON EXTENSION logical_ddl IS 'Catch DDL commands (CREATE, ALTER, DROP) for logging';

CREATE TABLE logical_ddl.ddl_commands (
    id SERIAL PRIMARY KEY,
    command_type TEXT NOT NULL,
    command_tag TEXT NOT NULL,
    schema_name TEXT,
    relation_name TEXT,
    command_text TEXT NOT NULL,
    executed_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
);


DO $$
BEGIN
    IF current_setting('logical_ddl.is_master', true)::boolean THEN
        CREATE PUBLICATION logical_ddl FOR TABLE logical_ddl.ddl_commands;
        RAISE NOTICE 'Publication logical_ddl created successfully.';
    END IF;
END $$;

DO $$
BEGIN
    IF NOT current_setting('logical_ddl.is_master', true)::boolean THEN
        CREATE SUBSCRIPTION logical_ddl_sub
        CONNECTION 'host=localhost port=5432 user=your_user password=your_password dbname=your_dbname'
        PUBLICATION logical_ddl;
        RAISE NOTICE 'Subscription logical_ddl_sub created successfully.';
    END IF;
END $$;

CREATE FUNCTION logical_ddl.ddl_command_trigger() 
RETURNS trigger
AS 'MODULE_PATHNAME', 'ddl_command_trigger'
LANGUAGE C VOLATILE STRICT;
