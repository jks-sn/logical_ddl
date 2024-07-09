ALTER TABLE logical_ddl.command_status DISABLE TRIGGER command_status_trigger;

DROP TRIGGER command_status_trigger ON logical_ddl.command_status;

DROP SUBSCRIPTION logical_ddl_status_sub;

DROP PUBLICATION logical_ddl_pub;

DROP TABLE logical_ddl.ddl_commands;

DROP TABLE logical_ddl.command_status;

DROP SCHEMA logical_ddl CASCADE;

DROP EXTENSION IF EXISTS logical_ddl CASCADE;

