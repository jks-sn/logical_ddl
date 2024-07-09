ALTER TABLE logical_ddl.ddl_commands DISABLE TRIGGER command_trigger;

DROP TRIGGER command_trigger ON logical_ddl.ddl_commands;

DROP SUBSCRIPTION logical_ddl_sub;

DROP PUBLICATION logical_ddl_status_pub;

DROP TABLE logical_ddl.ddl_commands;

DROP TABLE logical_ddl.command_status;

DROP SCHEMA logical_ddl CASCADE;

DROP EXTENSION logical_ddl CASCADE;
