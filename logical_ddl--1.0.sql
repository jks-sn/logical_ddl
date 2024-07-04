/* contrib/logical_ddl/logical_ddl--1.0.sql */

\echo Use "CREATE EXTENSION logical_ddl" to load this file. \quit

COMMENT ON EXTENSION logical_ddl IS 'Catch DDL commands (CREATE, ALTER, DROP) for logging';

CREATE TABLE logical_ddl.ddl_commands (
    id SERIAL PRIMARY KEY,
    classid OID NOT NULL,
    objid OID NOT NULL,
    objsubid INTEGER NOT NULL,
    command_tag TEXT NOT NULL,
    object_type TEXT NOT NULL,
    schema_name TEXT,
    object_identity TEXT NOT NULL,
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


CREATE OR REPLACE FUNCTION handle_ddl_command(
    classid OID,
    objid OID,
    objsubid INTEGER,
    command_tag TEXT,
    object_type TEXT,
    schema_name TEXT,
    object_identity TEXT,
    command_text TEXT
) RETURNS VOID
AS 'MODULE_PATHNAME', 'handle_ddl_command_c'
LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION capture_ddl_command() RETURNS event_trigger AS $$
DECLARE
    r RECORD;
BEGIN
    IF current_setting('logical_ddl.is_master', true)::boolean THEN
        FOR r IN
            SELECT * FROM pg_event_trigger_ddl_commands()
        LOOP
            IF r.in_extension THEN
                CONTINUE;
            END IF;

            PERFORM handle_ddl_command(
                r.classid,
                r.objid,
                r.objsubid,
                r.command_tag,
                r.object_type,
                r.schema_name,
                r.object_identity,
                r.command::text
            );
        END LOOP;
    END IF;
END;
$$ LANGUAGE plpgsql;

CREATE EVENT TRIGGER ddl_event_trigger
    ON ddl_command_end
    EXECUTE FUNCTION capture_ddl_command();
