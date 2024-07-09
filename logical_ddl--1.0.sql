/* contrib/logical_ddl/logical_ddl--1.0.sql */

\echo Use "CREATE EXTENSION logical_ddl" to load this file. \quit

COMMENT ON EXTENSION logical_ddl IS 'Catch DDL commands (CREATE, ALTER, DROP) for logging';

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
        CREATE TRIGGER command_status_trigger 
        AFTER INSERT ON logical_ddl.command_status 
        FOR EACH ROW 
        EXECUTE FUNCTION logical_ddl.command_status_trigger();

        ALTER TABLE logical_ddl.command_status ENABLE ALWAYS TRIGGER command_status_trigger;


    ELSE         
        CREATE TRIGGER command_trigger 
        AFTER INSERT ON logical_ddl.ddl_commands 
        FOR EACH ROW 
        EXECUTE FUNCTION logical_ddl.ddl_command_trigger();

        ALTER TABLE logical_ddl.ddl_commands ENABLE ALWAYS TRIGGER command_trigger;

    END IF;
END $$;
