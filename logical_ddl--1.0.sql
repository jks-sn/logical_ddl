/* contrib/logical_ddl/logical_ddl--1.0.sql */

\echo Use "CREATE EXTENSION logical_ddl" to load this file. \quit

DO $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_namespace WHERE nspname = 'logical_ddl') THEN
        CREATE SCHEMA logical_ddl;
        RAISE NOTICE 'Schema logical_ddl created.';
    END IF;
END $$;

COMMENT ON EXTENSION logical_ddl IS 'Catch DDL commands (CREATE, ALTER, DROP) for logging';

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



CREATE FUNCTION logical_ddl.ddl_command_trigger() 
RETURNS trigger
AS 'MODULE_PATHNAME', 'ddl_command_trigger'
LANGUAGE C VOLATILE STRICT;

CREATE FUNCTION logical_ddl.command_status_trigger() 
RETURNS trigger
AS 'MODULE_PATHNAME', 'command_status_trigger'
LANGUAGE C VOLATILE STRICT;

DO $$
BEGIN
    IF current_setting('logical_ddl.is_master', true)::boolean THEN

        IF NOT EXISTS (SELECT 1 FROM pg_publication WHERE pubname = 'logical_ddl_pub') THEN
            CREATE PUBLICATION logical_ddl_pub FOR TABLE logical_ddl.ddl_commands;
            RAISE NOTICE 'Publication logical_ddl_pub created successfully.';
        END IF;

        IF NOT EXISTS (SELECT 1 FROM pg_subscription WHERE subname = 'logical_ddl_status_sub') THEN
            ---PERFORM pg_create_logical_replication_slot('logical_ddl_sub_slot', 'pgoutput');
            CREATE SUBSCRIPTION logical_ddl_status_sub 
            CONNECTION 'host=localhost port=5433 dbname=postgres'
            PUBLICATION logical_ddl_status_pub 
            WITH (create_slot = false);
            RAISE NOTICE 'Subscription logical_ddl_status_sub created successfully.';
        END IF;

        CREATE TRIGGER command_status_trigger 
        AFTER INSERT ON logical_ddl.command_status 
        FOR EACH ROW 
        EXECUTE FUNCTION logical_ddl.command_status_trigger();

        ALTER TABLE logical_ddl.command_status ENABLE ALWAYS TRIGGER command_status_trigger ;

    ELSE         

        IF NOT EXISTS (SELECT 1 FROM pg_publication WHERE pubname = 'logical_ddl_status_pub') THEN
            CREATE PUBLICATION logical_ddl_status_pub FOR TABLE logical_ddl.command_status;
            RAISE NOTICE 'Publication logical_ddl_status_pub created successfully.';
        END IF;

        IF NOT EXISTS (SELECT 1 FROM pg_subscription WHERE subname = 'logical_ddl_sub') THEN
            ---PERFORM pg_create_logical_replication_slot('logical_ddl_status_sub_slot', 'pgoutput');
            CREATE SUBSCRIPTION logical_ddl_sub 
            CONNECTION 'host=localhost port=5432 dbname=postgres'
            PUBLICATION logical_ddl_pub 
            WITH (create_slot = false);
            RAISE NOTICE 'Subscription logical_ddl_sub created successfully.';
        END IF;

        CREATE TRIGGER ddl_command_trigger 
        AFTER INSERT ON logical_ddl.ddl_commands 
        FOR EACH ROW 
        EXECUTE FUNCTION logical_ddl.ddl_command_trigger();

        ALTER TABLE logical_ddl.ddl_commands ENABLE ALWAYS TRIGGER ddl_command_trigger;
    END IF;
END $$;
