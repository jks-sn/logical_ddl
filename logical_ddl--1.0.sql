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

ALTER TABLE logical_ddl.ddl_commands ENABLE TRIGGER ALL;

CREATE FUNCTION logical_ddl.ddl_command_trigger() 
RETURNS trigger
AS 'MODULE_PATHNAME', 'ddl_command_trigger'
LANGUAGE C VOLATILE STRICT;

DO $$
BEGIN
    IF current_setting('logical_ddl.is_master', true)::boolean THEN
        IF NOT EXISTS (
            SELECT 1 FROM pg_publication WHERE pubname = 'logical_ddl_pub'
        ) THEN
            CREATE PUBLICATION logical_ddl_pub FOR TABLE logical_ddl.ddl_commands;
            RAISE NOTICE 'Publication logical_ddl_pub created successfully.';
        ELSE
            RAISE NOTICE 'Publication logical_ddl_pub already exists.';
        END IF;

    ELSE
        IF NOT EXISTS (
            SELECT 1 FROM pg_subscription WHERE subname = 'logical_ddl_sub'
        ) THEN
            CREATE SUBSCRIPTION logical_ddl_sub
            CONNECTION 'host=localhost port=5432 dbname=postgres'
            PUBLICATION logical_ddl_pub
            WITH (create_slot = false);
            RAISE NOTICE 'Subscription logical_ddl_sub created successfully.';
        ELSE
            RAISE NOTICE 'Subscription logical_ddl_sub already exists.';
        END IF;

        CREATE TRIGGER ddl_command_trigger 
        AFTER INSERT ON logical_ddl.ddl_commands 
        FOR EACH ROW 
        EXECUTE FUNCTION logical_ddl.ddl_command_trigger();
    END IF;
END $$;
