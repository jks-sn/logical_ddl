#ifndef DDL_COMMANDS_H
#define DDL_COMMANDS_H

#include "postgres.h"

void insert_ddl_command(Oid classid, Oid objid, int32 objsubid, const char *commandTag, const char *objectType, const char *schemaName, const char *objectIdentity, const char *commandText);
void execute_ddl_command(int32 id, const char *command_text);

Datum ddl_command_trigger(PG_FUNCTION_ARGS);

#endif // DDL_COMMANDS_H
