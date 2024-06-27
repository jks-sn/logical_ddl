/* contrib/logical_ddl/logical_ddl--1.0.sql */

\echo Use "CREATE EXTENSION logical_ddl" to load this file. \quit

COMMENT ON EXTENSION logical_ddl IS 'Catch DDL commands (CREATE, ALTER, DROP) for logging';

CREATE TABLE logical_ddl.ddl_commands (
    id SERIAL PRIMARY KEY,
    command_type TEXT NOT NULL,
    command_tag TEXT NOT NULL,
    command_text TEXT NOT NULL,
    executed_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
);

/*CREATE OR REPLACE FUNCTION logical_ddl.log_ddl_command()
RETURNS event_trigger AS $$
DECLARE
    command_type TEXT;
    command_tag TEXT;
BEGIN
    CASE
        WHEN TG_TAG = 'CREATE TABLE' THEN command_type := 'create'; command_tag := 'CREATE TABLE';
        WHEN TG_TAG = 'ALTER TABLE' THEN command_type := 'alter'; command_tag := 'ALTER TABLE';
        WHEN TG_TAG = 'DROP TABLE' THEN command_type := 'drop'; command_tag := 'DROP TABLE';
        ELSE command_type := 'other'; command_tag := TG_TAG;
    END CASE;
$$ LANGUAGE plpgsql;*/

CREATE OR REPLACE FUNCTION logical_ddl.set_master(master BOOLEAN) RETURNS VOID AS $$
BEGIN
    PERFORM set_master_c(master);
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION logical_ddl.get_master() RETURNS BOOLEAN AS $$
BEGIN
    RETURN get_master_c();
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION logical_ddl.ddl_command_trigger() RETURNS trigger AS $$
BEGIN
    PERFORM logical_ddl.ddl_command_trigger(NEW.id, NEW.command_text);
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER ddl_command_trigger
AFTER INSERT ON logical_ddl.ddl_commands
FOR EACH ROW
EXECUTE FUNCTION logical_ddl.ddl_command_trigger();
